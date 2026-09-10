#pragma once

#include <memory>

namespace gfx {
namespace rhi {
class Device;
}

class Renderer {
 public:
  Renderer();
  ~Renderer();
  void init();
  void render();

 private:
  std::unique_ptr<gfx::rhi::Device> device_;
};
}  // namespace gfx
