#include "Renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include <fstream>

#include "core/EAssert.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

namespace {

std::vector<char> read_file_to_bytes(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  std::vector<char> bytes;
  if (!file.is_open()) {
    return bytes;
  }
  auto file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  bytes.resize(file_size);
  file.read(bytes.data(), file_size);
  return bytes;
}

}  // namespace

namespace gfx {

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::init(const InitInfo& info) {
  window_ = info.window;
  shader_dir_ = info.shader_dir;

  device_ = make_device();
  device_->init();
  {
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    device_->create_swapchain(
        rhi::SwapchainDesc{.width = static_cast<uint32_t>(w), .height = static_cast<uint32_t>(h)},
        window_, swapchain_);
  }

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
  struct Job {
    rhi::Shader* shader;
    std::string path;
  };
  Job jobs[] = {Job{.shader = &basic_vs_, .path = "basic.vs"},
                Job{.shader = &basic_fs_, .path = "basic.fs"}};

  for (auto& job : jobs) {
    std::filesystem::path full_path = shader_dir_ / job.path;
    load_shader(*job.shader, full_path);
  }
}

void Renderer::load_shader(rhi::Shader& shader, const std::string& path) {
  auto bytes = read_file_to_bytes(path);
  ASSERT(bytes.size());

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

  if (bytes.size()) {
    device_->create_shader(type, bytes.data(), bytes.size(), shader);
  }
}

}  // namespace gfx
