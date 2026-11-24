#include "Renderer.hpp"

#include "basic_shared.h"
#include "forward_pass_shared.h"

#include "core/Logger.hpp"
#include "core/Util.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/rhi/ShaderType.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"
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

} // namespace

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::init(const InitInfo& info) {
  window_ = info.window;

  device_ = make_device();
  device_->init();

  {
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    device_->create_swapchain(
        {.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)}, window_,
        swapchain_);
  }

  FATAL_IF(info.shader_root.empty(), "shader_root is empty");
  FATAL_IF(info.cache_root.empty(), "cache_root is empty");
  shader_cache_.emplace(ShaderCache::CacheRoots{
      .shader_root = info.shader_root,
      .cache_root = info.cache_root,
  });

  reload_shaders();

  {
    // TODO: remove this shit
    struct Vertex {
      glm::vec3 position;
    };

    Vertex verts[] = {
        {{-0.5, -0.5, 0}},
        {{0.5, -0.5, 0}},
        {{0, 0.5, 0}},
    };
    device_->create_buffer(
        {.size = sizeof(Vertex) * ARRAY_SIZE(verts), .memory_type = MemoryType::CpuVisible},
        vert_buf_);
    memcpy(vert_buf_.cpu_ptr, verts, sizeof(Vertex) * ARRAY_SIZE(verts));
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
  {
    // swapchain resize
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    if (w != swapchain_.desc.width || h != swapchain_.desc.height) {
      device_->create_swapchain(
          {.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)}, window_,
          swapchain_);
    }
  }

  size_t frame_in_flight = frame_num_ % frames_in_flight_;
  auto& frame_allocator = frame_allocators_[frame_in_flight];
  frame_allocator->reset();

  auto* enc = device_->begin_cmd_encoder();

  enc->bind_pipeline(basic_comp_pso_);

  auto compute_root_data = frame_allocator->allocate<BasicRoot>(1);
  *compute_root_data.cpu = BasicRoot{
      .output_result = static_cast<float*>(test_gpu_allocation_buffer_.gpu_ptr),
      .color_x = std::sin(SDL_GetTicksNS() / 1000.f / 1000.f / 1000.f * 10),
  };

  enc->dispatch(compute_root_data.gpu, 1, 1, 1);
  enc->barrier();

  enc->begin_rendering(swapchain_);
  enc->bind_pipeline(basic_);

  auto forward_root_data = frame_allocator->allocate<ForwardRoot>(1);
  *forward_root_data.cpu = ForwardRoot{
      .output_result = static_cast<float*>(test_gpu_allocation_buffer_.gpu_ptr),
      .vertices = reinterpret_cast<Vertex*>(vert_buf_.gpu_ptr),
      .padding0 = 0,
  };

  enc->draw(forward_root_data.gpu, 3, 1, 0, 0);

  enc->end_rendering();

  device_->end_cmd_encoder(enc);
  device_->submit_queue();

  frame_num_++;
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

  device_->create_pipeline({.vertex_shader = &basic_vs_, .fragment_shader = &basic_fs_}, basic_);
  device_->create_pipeline(basic_cs_, basic_comp_pso_);
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
