#pragma once

#include <memory>

#include "graphics/rhi/Graphics.hpp"

namespace gfx {
namespace rhi {
class Device;
}

class Renderer {
 public:
  Renderer();
  ~Renderer();
  void init(SDL_Window* window);
  void render();

 private:
  SDL_Window* window_;
  std::unique_ptr<gfx::rhi::Device> device_;
  gfx::rhi::Swapchain swapchain_;
};
}  // namespace gfx
