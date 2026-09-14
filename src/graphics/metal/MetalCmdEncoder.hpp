#pragma once

// clang-format off
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <Metal/MTL4ComputeCommandEncoder.hpp>
#include <Metal/MTL4RenderCommandEncoder.hpp>
#include <Metal/MTLDrawable.hpp>
#include <cstddef>

#include "graphics/rhi/CmdEncoder.hpp"
#include "small_vector/small_vector.hpp"

namespace gfx::metal {

class MetalCmdEncoder : public rhi::CmdEncoder {
 public:
  size_t get_encoder_index() { return 0; }

  void set_cmd_buffer(const NS::SharedPtr<MTL4::CommandBuffer>& cmd_buf) { cmd_buf_ = cmd_buf; }

  void begin_rendering(rhi::Swapchain& swapchain) override;
  void end_rendering() override;
  void bind_pipeline(rhi::Pipeline& pipeline) override;
  void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
            uint32_t first_instance) override;

  NS::SharedPtr<MTL4::RenderCommandEncoder> curr_render_encoder_;
  NS::SharedPtr<MTL4::ComputeCommandEncoder> curr_compute_encoder_;
  NS::SharedPtr<MTL4::CommandBuffer> cmd_buf_;

  gch::small_vector<NS::SharedPtr<MTL::Drawable>, 8> presents_;
};

}  // namespace gfx::metal
