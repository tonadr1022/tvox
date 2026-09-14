#pragma once

// clang-format off
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <optional>
#include <string_view>
#include <unordered_map>

#include <Metal/Metal.hpp>

#include "graphics/metal/MetalCmdEncoder.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "small_vector/small_vector.hpp"

namespace MTL {
class Device;
class RenderPipelineState;
}  // namespace MTL

namespace gfx::metal {

class MetalDevice : public gfx::rhi::Device {
 public:
  ~MetalDevice() override;

  void init() override;

  void create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                        rhi::Swapchain& swapchain) override;
  void create_pipeline(rhi::GraphicsPipelineCreateInfo& cinfo) override;
  bool create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) override;
  bool create_shader(rhi::ShaderType type, const void* data, size_t size, rhi::Shader& shader,
                     std::string_view entry_point) override;

  rhi::CmdEncoder* begin_cmd_encoder() override;
  void end_cmd_encoder(rhi::CmdEncoder* encoder) override;

  void submit_queue() override;

  /// Compile (or fetch cached) MTL render pipeline for the active render-pass formats.
  MTL::RenderPipelineState* ensure_render_pipeline(rhi::Pipeline& pipeline,
                                                   const rhi::RenderPassInfo& renderpass_info);

 private:
  struct PipelineHashHasher {
    size_t operator()(const rhi::PipelineHash& hash) const {
      return static_cast<size_t>(hash.get_hash());
    }
  };

  NS::SharedPtr<MTL::Device> device_;
  NS::SharedPtr<MTL4::CommandQueue> queue_;

  std::unordered_map<rhi::PipelineHash, NS::SharedPtr<MTL::RenderPipelineState>, PipelineHashHasher>
      pipelines_cache_;

  struct PerFrame {
    NS::SharedPtr<MTL4::CommandAllocator> cmd_allocator;
    NS::SharedPtr<MTL4::CommandBuffer> cmd_buf;
    std::optional<MetalCmdEncoder> cmd_encoder;
    size_t cmd_buffers_allocated{};
    NS::SharedPtr<MTL::SharedEvent> fence;
    size_t fence_value{};
  };

  [[nodiscard]] size_t curr_frame_in_flight() const { return frame_num_ % frames_in_flight_; }
  PerFrame& curr_frame() { return per_frame_[curr_frame_in_flight()]; }

  gch::small_vector<PerFrame, rhi::k_max_frames_in_flight> per_frame_;

  size_t frames_in_flight_{3};
  size_t frame_num_{};
};

}  // namespace gfx::metal
