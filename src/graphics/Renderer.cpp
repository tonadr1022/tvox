#include "Renderer.hpp"

#include "basic_shared.h"
#include "forward_pass_shared.h"

#include "core/Logger.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "graphics/ModelLoad.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/rhi/ShaderType.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"
#include "post_process_shared.h"
#include "vertex_shared.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <cstddef>

namespace gfx {

using namespace rhi;

namespace {

const ShaderTechniqueDesc::StageDesc* find_stage(const ShaderTechniqueDesc& tech,
                                                 ShaderType stage) {
  for (const ShaderTechniqueDesc::StageDesc& desc : tech.stages) {
    if (desc.stage == stage) {
      return &desc;
    }
  }
  return nullptr;
}

uint64_t last_time{};
Model model_res;

} // namespace

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::init(const InitInfo& info) {
  window_ = info.window;

  device_ = make_device();
  device_->init({
      .desired_generic_queue_count = 1,
      .desired_compute_only_queue_count = 0,
      .desired_copy_only_queue_count = 1,
  });
  upload_queue_.init(device_.get(), 64ull * 1024 * 1024, 3);

  model_res = load_model("/Users/tony/personal/tvox/resources/models/Cube/glTF/Cube.gltf");

  device_->create_timeline_semaphore(latest_completion_sema_.sema);
  texture_descriptor_index_allocator_.init(
      512); // TODO: don't hardcode this. might need to make desc heap public

  {
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    device_->create_swapchain(
        {.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)}, window_,
        swapchain_);
    recreate_textures();
  }

  FATAL_IF(info.shader_root.empty(), "shader_root is empty");
  FATAL_IF(info.cache_root.empty(), "cache_root is empty");
  shader_cache_.emplace(ShaderCache::CacheRoots{
      .shader_root = info.shader_root,
      .cache_root = info.cache_root,
  });

  reload_shaders();

  for (auto& cmd_pool : cmd_pools_) {
    device_->create_cmd_pool(cmd_pool, 0);
  }

  {
    auto& verts = model_res.vertices;
    device_->create_buffer(
        {.size = sizeof(Vertex) * verts.size(), .memory_type = MemoryType::GpuOnly}, vert_buf_);
    device_->create_buffer(
        {.size = sizeof(uint32_t) * model_res.indices.size(), .memory_type = MemoryType::GpuOnly},
        index_buf_);
    upload_queue_.upload_to_buffer(vert_buf_, 0,
                                   {reinterpret_cast<std::byte*>(model_res.vertices.data()),
                                    model_res.vertices.size() * sizeof(Vertex)});
    upload_queue_.upload_to_buffer(index_buf_, 0,
                                   {reinterpret_cast<std::byte*>(model_res.indices.data()),
                                    model_res.indices.size() * sizeof(uint32_t)});
  }

  for (size_t i = 0; i < k_max_frames_in_flight; i++) {
    device_->create_buffer({.size = 1024ull * 1024, .memory_type = MemoryType::CpuVisible},
                           frame_scratch_buffers_[i]);
    frame_allocators_[i].emplace(gpu_cpu_range(frame_scratch_buffers_[i]));
  }

  device_->create_buffer({.size = 1024ull * 1024, .memory_type = MemoryType::GpuOnly},
                         test_gpu_allocation_buffer_);
}

void Renderer::render() {
  if (latest_completion_sema_.value > frames_in_flight_) {
    device_->wait_timeline_semaphore(latest_completion_sema_.sema,
                                     latest_completion_sema_.value - frames_in_flight_ + 1);
  }

  auto upload_timeline_pt = upload_queue_.flush();

  int w{}, h{};
  SDL_GetWindowSize(window_, &w, &h);
  if (w <= 0 || h <= 0) {
    return;
  }

  {
    // swapchain resize
    if (w != swapchain_.desc.width || h != swapchain_.desc.height) {
      device_->create_swapchain(
          {
              .width = static_cast<uint32_t>(w),
              .height = static_cast<uint32_t>(h),
          },
          window_, swapchain_);
      recreate_textures();
    }
  }

  float aspect = static_cast<float>(w) / static_cast<float>(h);

  glm::mat4 projection_matrix = glm::perspective(glm::radians(70.f), aspect, 0.1f, 500.f);
  glm::mat4 view_matrix = glm::lookAt(glm::vec3{0, 5, 5}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});

  size_t frame_in_flight = frame_num_ % frames_in_flight_;
  auto& frame_allocator = frame_allocators_[frame_in_flight];
  frame_allocator->reset();

  device_->reset_cmd_pool(cmd_pools_[frame_in_flight]);
  auto* enc = device_->begin_cmd_encoder(cmd_pools_[frame_in_flight]);

  enc->bind_pipeline(basic_comp_pso_);

  auto compute_root_data = frame_allocator->allocate<BasicRoot>(1);
  *compute_root_data.cpu = BasicRoot{
      .output_result = static_cast<float*>(test_gpu_allocation_buffer_.gpu_ptr),
      .color_x = std::sin(SDL_GetTicksNS() / 1000.f / 1000.f / 1000.f * 10),
  };

  enc->dispatch(compute_root_data.gpu, 1, 1, 1);
  enc->barrier(rhi::Stage::Compute, rhi::Access::ShaderWrite, rhi::Stage::Vertex,
               rhi::Access::ShaderRead);

  {
    auto depth_att = RenderAttInfo::depth_stencil_att({.tex = &depth_att_tex_}, LoadOp::DontCare);
    RenderAttInfo color_atts[] = {
        RenderAttInfo::color_att({.tex = &color_att_tex_}, LoadOp::DontCare,
                                 {.color = glm::vec4{1, 0.5, 0.5, 1}}),
    };
    enc->begin_rendering(w, h, color_atts, &depth_att);
    enc->bind_pipeline(basic_);
    auto forward_root_data = frame_allocator->allocate<ForwardRoot>(1);
    *forward_root_data.cpu = ForwardRoot{
        .view_proj = glm::transpose(projection_matrix * view_matrix),
        .output_result = static_cast<float*>(test_gpu_allocation_buffer_.gpu_ptr),
        .vertices = reinterpret_cast<Vertex*>(vert_buf_.gpu_ptr),
        .padding0 = 0,
    };
    enc->draw_indexed(forward_root_data.gpu, model_res.indices.size(), index_buf_);
    enc->end_rendering();
    enc->barrier(rhi::Stage::ColorOutput | rhi::Stage::DepthStencilTests,
                 rhi::Access::ColorWrite | rhi::Access::DepthStencilWrite, rhi::Stage::Fragment,
                 rhi::Access::ShaderRead);
  }

  {
    enc->begin_rendering(swapchain_);
    enc->bind_pipeline(post_process_);

    auto post_process_root = frame_allocator->allocate<PostProcessRoot>(1);
    *post_process_root.cpu = PostProcessRoot{
        .mult = std::sin(SDL_GetTicksNS() / 1000.f / 1000.f / 1000.f * 10),
        .tex = color_att_tex_.index,
    };
    enc->draw(post_process_root.gpu, 3, 1, 0, 0);
    enc->end_rendering();
  }

  device_->end_cmd_encoder(enc);

  rhi::CmdEncoder* cmds[] = {enc};

  frame_num_++;
  latest_completion_sema_.value++;

  const rhi::TimelinePoint waits[] = {upload_timeline_pt};
  device_->submit(cmds, waits, latest_completion_sema_);
}

void Renderer::reload_shaders() {
  const auto stats = shader_cache_->ensure_all();
  LINFO("shader cache: {} compiled, {} up to date, {} failed", stats.compiled, stats.up_to_date,
        stats.failed);
  if (stats.failed > 0) {
    LERROR("shader ensure_all failed: {}", stats.first_error);
  }

  load_shader(basic_vs_, "basic_tri", ShaderType::Vertex);
  load_shader(basic_fs_, "basic_tri", ShaderType::Fragment);
  load_shader(basic_cs_, "basic", ShaderType::Compute);
  load_shader(post_process_vs_, "post_process", ShaderType::Vertex);
  load_shader(post_process_fs_, "post_process", ShaderType::Fragment);

  device_->create_pipeline({.vertex_shader = &basic_vs_, .fragment_shader = &basic_fs_}, basic_);
  device_->create_pipeline(basic_cs_, basic_comp_pso_);
  device_->create_pipeline(
      {.vertex_shader = &post_process_vs_, .fragment_shader = &post_process_fs_}, post_process_);
}

void Renderer::recreate_textures() {
  int w{}, h{};
  SDL_GetWindowSize(window_, &w, &h);

  auto create_texture = [this](BindlessTexture& texture, const rhi::TextureDesc& desc) {
    device_->create_texture(texture, desc);
    texture.index = texture_descriptor_index_allocator_.alloc();
    device_->write_texture_desc({.texture_type = texture.desc.type}, color_att_tex_, texture.index);
  };

  create_texture(color_att_tex_, {
                                     .width = static_cast<uint32_t>(w),
                                     .height = static_cast<uint32_t>(h),
                                     .depth = 1,
                                     .format = rhi::Format::Rgba32Float,
                                     .usage = TextureUsage::RenderTarget,
                                 });
  create_texture(depth_att_tex_, {
                                     .width = static_cast<uint32_t>(w),
                                     .height = static_cast<uint32_t>(h),
                                     .depth = 1,
                                     .format = rhi::Format::D32Float,
                                     .usage = TextureUsage::RenderTarget,
                                 });
}

void Renderer::load_shader(Shader& shader, std::string_view technique, ShaderType stage) {
  const ShaderTechniqueDesc* tech = ShaderTechniqueRegistry::find(technique);
  FATAL_IF(!tech, "unknown technique '{}'", technique);

  const ShaderTechniqueDesc::StageDesc* stage_desc = find_stage(*tech, stage);
  if (!stage_desc) {
    LERROR("technique '{}' has no requested stage", technique);
    return;
  }

  const auto bytes = shader_cache_->load_shader(technique, stage);
  if (!bytes) {
    LERROR("failed to load metallib for {} / {}: {}", technique, stage_desc->entry, bytes.error());
    return;
  }

  if (!device_->create_shader(stage, bytes->data(), bytes->size(), shader, stage_desc->entry)) {
    LERROR("create_shader failed for {} / {}", technique, stage_desc->entry);
  }
}

} // namespace gfx
