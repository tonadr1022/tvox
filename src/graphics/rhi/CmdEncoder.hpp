#pragma once

#include "graphics/rhi/Graphics.hpp"

namespace gfx::rhi {
class CmdEncoder {
public:
  virtual ~CmdEncoder() = default;
  virtual void begin_rendering(rhi::Swapchain& swapchain) = 0;
  virtual void end_rendering() = 0;
  virtual void bind_pipeline(rhi::Pipeline& pipeline) = 0;

  virtual void draw(const void* root, uint32_t vertex_count, uint32_t instance_count,
                    uint32_t first_vertex, uint32_t first_instance) = 0;
  virtual void draw_indexed(const void* root, uint32_t index_count, rhi::Buffer& index_buffer) = 0;
  virtual void dispatch(const void* root, uint32_t thread_groups_x, uint32_t thread_groups_y,
                        uint32_t thread_groups_z) = 0;
  virtual void copy_buffer_to_buffer(rhi::Buffer& dst, size_t dst_offset, rhi::Buffer& src,
                                     size_t src_offset, size_t size) = 0;

  virtual void barrier() = 0;
};
} // namespace gfx::rhi
