#pragma once

#include <filesystem>
#include <memory>

#include "graphics/rhi/Graphics.hpp"
#include "graphics/shaders/ShaderCache.hpp"

namespace gfx {
namespace rhi {
class Device;
}

class Renderer {
 public:
  Renderer();
  ~Renderer();
  struct InitInfo {
    SDL_Window* window{};
    std::filesystem::path shader_root;
    std::filesystem::path cache_root;
  };
  void init(const InitInfo& info);
  void render();

 private:
  void reload_shaders();
  void load_shader(rhi::Shader& shader, const std::string& path);
  SDL_Window* window_{};
  std::optional<ShaderCache> shader_cache_;
  std::unique_ptr<gfx::rhi::Device> device_;
  gfx::rhi::Swapchain swapchain_;
  rhi::Shader basic_vs_;
  rhi::Shader basic_fs_;
};
}  // namespace gfx
