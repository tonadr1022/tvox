#pragma once

// clang-format off
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
  size_t get_encoder_index() {
    return 0;
  }

  void init(MetalDevice* device, const NS::SharedPtr<MTL4::CommandBuffer>& cmd_buf);

  void begin_rendering(rhi::Swapchain& swapchain) override;
  void end_rendering() override;
  void bind_pipeline(rhi::Pipeline& pipeline) override;

  void draw(const void* root, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
            uint32_t first_instance) override;
  void dispatch(const void* root, uint32_t thread_groups_x, uint32_t thread_groups_y,
                uint32_t thread_groups_z) override;
  void barrier() override;
  void copy_buffer_to_buffer(rhi::Buffer& dst, size_t dst_offset, rhi::Buffer& src,
                             size_t src_offset, size_t size) override;

  NS::SharedPtr<MTL4::RenderCommandEncoder> curr_render_encoder_;
  NS::SharedPtr<MTL4::ComputeCommandEncoder> curr_compute_encoder_;
  NS::SharedPtr<MTL4::CommandBuffer> cmd_buf_;
  NS::SharedPtr<MTL4::ArgumentTable> arg_table_;

  gch::small_vector<NS::SharedPtr<MTL::Drawable>, 8> presents_;

private:
  void set_root(const void* root) const;
  void end_compute();
  void start_compute();

  MetalDevice* device_{};
  rhi::RenderPassInfo renderpass_info_{};
};

} // namespace gfx::metal
