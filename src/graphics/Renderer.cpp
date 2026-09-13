#include "Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "core/Logger.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"

namespace gfx {
namespace {

const ShaderTechniqueDesc::StageDesc* find_stage(const ShaderTechniqueDesc& tech,
                                                 rhi::ShaderType stage) {
  for (const ShaderTechniqueDesc::StageDesc& desc : tech.stages) {
    if (desc.stage == stage) {
      return &desc;
    }
  }
  return nullptr;
}

}  // namespace

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
        rhi::SwapchainDesc{.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)},
        window_, swapchain_);
  }

  FATAL_IF(info.shader_root.empty(), "shader_root is empty");
  FATAL_IF(info.cache_root.empty(), "cache_root is empty");
  shader_cache_.emplace(ShaderCache::CacheRoots{
      .shader_root = info.shader_root,
      .cache_root = info.cache_root,
  });

  reload_shaders();
}

void Renderer::render() {
  {
    // swapchain resize
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    if (w != swapchain_.desc.width || h != swapchain_.desc.height) {
      device_->create_swapchain(
          rhi::SwapchainDesc{.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)},
          window_, swapchain_);
    }
  }

  auto* enc = device_->begin_cmd_encoder();
  enc->begin_rendering(swapchain_);

  enc->end_rendering();
  device_->end_cmd_encoder(enc);
  device_->submit_queue();
}

void Renderer::reload_shaders() {
  const auto stats = shader_cache_->ensure_all();
  LINFO("shader cache: {} compiled, {} up to date, {} failed", stats.compiled, stats.up_to_date,
        stats.failed);
  if (stats.failed > 0) {
    LERROR("shader ensure_all failed: {}", stats.first_error);
  }

  load_shader(basic_vs_, "basic", rhi::ShaderType::Vertex);
  load_shader(basic_fs_, "basic", rhi::ShaderType::Fragment);
}

void Renderer::load_shader(rhi::Shader& shader, std::string_view technique, rhi::ShaderType stage) {
  const ShaderTechniqueDesc* tech = ShaderTechniqueRegistry::find(technique);
  if (!tech) {
    LERROR("unknown technique '{}'", technique);
    return;
  }

  const ShaderTechniqueDesc::StageDesc* stage_desc = find_stage(*tech, stage);
  if (!stage_desc) {
    LERROR("technique '{}' has no requested stage", technique);
    return;
  }

  std::vector<uint8_t> bytes;
  std::string error;
  if (!shader_cache_->load_shader(technique, stage, bytes, &error)) {
    LERROR("failed to load metallib for {} / {}: {}", technique, stage_desc->entry, error);
    return;
  }

  if (!device_->create_shader(stage, bytes.data(), bytes.size(), shader, stage_desc->entry)) {
    LERROR("create_shader failed for {} / {}", technique, stage_desc->entry);
  }
}

}  // namespace gfx
