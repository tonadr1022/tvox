#pragma once

#include "graphics/rhi/Graphics.hpp"

namespace gfx::rhi {
class CmdEncoder {
 public:
  virtual ~CmdEncoder() = default;
  virtual void begin_rendering(rhi::Swapchain& swapchain) = 0;
  virtual void end_rendering() = 0;
  virtual void bind_pipeline(rhi::Pipeline& pipeline) = 0;
  virtual void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
                    uint32_t first_instance) = 0;

 private:
};
}  // namespace gfx::rhi
