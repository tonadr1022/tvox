#include "Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "core/FileIo.hpp"
#include "core/Logger.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

namespace gfx {

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

  // TODO: Metallib load-by-logical-id + entry-point wiring is the next todo.
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

  struct Job {
    rhi::Shader* shader;
    std::string path;
  };
  Job jobs[] = {Job{.shader = &basic_vs_, .path = "basic.vs"},
                Job{.shader = &basic_fs_, .path = "basic.fs"}};

  for (auto& job : jobs) {
    // TODO: Fix
    // std::filesystem::path full_path = shader_root_ / job.path;
    // load_shader(*job.shader, full_path.string());
  }
}

void Renderer::load_shader(rhi::Shader& shader, const std::string& path) {
  std::vector<uint8_t> bytes;
  std::string error;
  if (!core::read_bytes(path, bytes, error)) {
    LERROR("{}", error);
    return;
  }

  rhi::ShaderType type{rhi::ShaderType::None};
  if (path.ends_with(".vs")) {
    type = rhi::ShaderType::Vertex;
  }
  if (path.ends_with(".ms")) {
    type = rhi::ShaderType::Mesh;
  }
  if (path.ends_with(".ts")) {
    type = rhi::ShaderType::Task;
  }
  if (path.ends_with(".fs")) {
    type = rhi::ShaderType::Fragment;
  }
  if (path.ends_with(".cs")) {
    type = rhi::ShaderType::Compute;
  }

  device_->create_shader(type, bytes.data(), bytes.size(), shader);
}

}  // namespace gfx
