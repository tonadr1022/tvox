#pragma once

#include "graphics/rhi/Graphics.hpp"
#include <glm/vec3.hpp>

namespace gfx::rhi {
class CmdEncoder {
public:
  virtual ~CmdEncoder() = default;
  virtual void begin_rendering(rhi::Swapchain& swapchain) = 0;
  virtual void begin_rendering(uint32_t width, uint32_t height,
                               std::span<rhi::RenderAttInfo> color_atts,
                               rhi::RenderAttInfo* depth_att) = 0;
  virtual void end_rendering() = 0;
  virtual void bind_pipeline(rhi::Pipeline& pipeline) = 0;

  virtual void draw(const void* root, uint32_t vertex_count, uint32_t instance_count,
                    uint32_t first_vertex, uint32_t first_instance) = 0;
  virtual void draw_indexed(const void* root, uint32_t index_count, rhi::Buffer& index_buffer) = 0;
  virtual void dispatch(const void* root, uint32_t thread_groups_x, uint32_t thread_groups_y,
                        uint32_t thread_groups_z) = 0;
  virtual void copy_buffer_to_buffer(rhi::Buffer& dst, uint32_t dst_offset, rhi::Buffer& src,
                                     uint32_t src_offset, uint32_t size) = 0;
  virtual void copy_texture_from_buffer(rhi::Texture& dst, uint32_t dst_slice, uint32_t dst_mip,
                                        rhi::Buffer& src, uint32_t src_offset,
                                        uint32_t bytes_per_row, glm::uvec3 src_size,
                                        glm::uvec3 dst_origin) = 0;

  virtual void barrier(Stage before, Access before_access, Stage after, Access after_access) = 0;
};
} // namespace gfx::rhi
