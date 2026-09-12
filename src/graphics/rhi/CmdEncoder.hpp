#pragma once

#include "graphics/rhi/Graphics.hpp"

namespace gfx::rhi {
class CmdEncoder {
 public:
  virtual ~CmdEncoder() = default;
  virtual void begin_rendering(rhi::Swapchain& swapchain) = 0;
  virtual void end_rendering() = 0;

 private:
};
}  // namespace gfx::rhi
