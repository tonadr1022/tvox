#pragma once

// clang-format off
#include <Metal/MTL4CommandAllocator.hpp>
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "small_vector/small_vector.hpp"

#include <Metal/MTL4ComputeCommandEncoder.hpp>
#include <Metal/MTL4RenderCommandEncoder.hpp>
#include <Metal/MTLDrawable.hpp>
#include <cstddef>

namespace gfx::metal {

class MetalDevice;

class MetalCmdEncoder : public rhi::CmdEncoder {
public:
  ~MetalCmdEncoder() override;
  void begin_rendering(rhi::Swapchain& swapchain) override;
  void begin_rendering(uint32_t width, uint32_t height, std::span<rhi::RenderAttInfo> color_atts,
                       rhi::RenderAttInfo* depth_att) override;
  void end_rendering() override;
  void bind_pipeline(rhi::Pipeline& pipeline) override;

  void draw(const void* root, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
            uint32_t first_instance) override;
  void draw_indexed(const void* root, uint32_t index_count, rhi::Buffer& index_buffer) override;
  void dispatch(const void* root, uint32_t thread_groups_x, uint32_t thread_groups_y,
                uint32_t thread_groups_z) override;
  void barrier(rhi::Stage before, rhi::Access before_access, rhi::Stage after,
               rhi::Access after_access) override;
  void copy_buffer_to_buffer(rhi::Buffer& dst, size_t dst_offset, rhi::Buffer& src,
                             size_t src_offset, size_t size) override;

  NS::SharedPtr<MTL4::RenderCommandEncoder> curr_render_encoder_;
  NS::SharedPtr<MTL4::ComputeCommandEncoder> curr_compute_encoder_;
  NS::SharedPtr<MTL4::ArgumentTable> arg_table_;

  gch::small_vector<NS::SharedPtr<MTL::Drawable>, 8> presents_;

  NS::SharedPtr<MTL4::CommandAllocator> cmd_allocator_;
  NS::SharedPtr<MTL4::CommandBuffer> cmd_buf_;
  MetalDevice* device_{};
  rhi::RenderPassInfo renderpass_info_{};

  struct PendingBarrier {
    rhi::Stage before;
    rhi::Access before_access;
    rhi::Stage after;
    rhi::Access after_access;
  };
  gch::small_vector<PendingBarrier, 8> pending_barriers_;

  void end_compute();

private:
  friend class MetalDevice;

  void set_root(const void* root) const;
  void start_compute();
  void flush_pending_barriers();
};

} // namespace gfx::metal
