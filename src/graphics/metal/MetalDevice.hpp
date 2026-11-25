#pragma once

// clang-format off
#include <Metal/MTLResidencySet.hpp>
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include "graphics/metal/MetalCmdEncoder.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "small_vector/small_vector.hpp"

#include <Metal/Metal.hpp>
#include <string_view>
#include <unordered_map>

namespace MTL {
class Device;
class RenderPipelineState;
} // namespace MTL

namespace gfx::metal {

class MetalDevice : public gfx::rhi::Device {
public:
  ~MetalDevice() override;

  void init() override;

  void create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                        rhi::Swapchain& swapchain) override;
  bool create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) override;
  bool create_pipeline(const rhi::Shader& compute_shader, rhi::Pipeline& pipeline) override;
  bool create_gpu_heap(const rhi::GpuHeapDesc& desc, rhi::GpuHeap& heap) override;
  bool create_shader(rhi::ShaderType type, const void* data, size_t size, rhi::Shader& shader,
                     std::string_view entry_point) override;
  bool create_buffer(const rhi::BufferDesc& desc, rhi::Buffer& buffer) override;
  bool create_cmd_pool(rhi::CmdPool& pool) override;
  void reset_cmd_pool(rhi::CmdPool& pool) override;

  rhi::CmdEncoder* begin_cmd_encoder(rhi::CmdPool& pool) override;
  void end_cmd_encoder(rhi::CmdEncoder* encoder) override;

  MTL::Device* get_device() const {
    return device_.get();
  }

  void submit(std::span<rhi::CmdEncoder*> cmds) override;

  MTL::RenderPipelineState*
  get_or_create_render_pipeline(rhi::Pipeline& pipeline,
                                const rhi::RenderPassInfo& renderpass_info);

private:
  struct PipelineHashHasher {
    size_t operator()(const rhi::PipelineHash& hash) const {
      return static_cast<size_t>(hash.get_hash());
    }
  };
  void make_resident(MTL::Allocation* buf);

  NS::SharedPtr<MTL::Device> device_;
  NS::SharedPtr<MTL4::CommandQueue> queue_;

  std::unordered_map<rhi::PipelineHash, NS::SharedPtr<MTL::RenderPipelineState>, PipelineHashHasher>
      pipelines_cache_;

  struct PerFrame {
    NS::SharedPtr<MTL::SharedEvent> fence;
    size_t fence_value{};
  };

  // TODO: idt the device should have a notion of "current frame"
  [[nodiscard]]
  size_t curr_frame_in_flight() const {
    return frame_num_ % frames_in_flight_;
  }

  PerFrame& curr_frame() {
    return per_frame_[curr_frame_in_flight()];
  }

  gch::small_vector<PerFrame, rhi::k_max_frames_in_flight> per_frame_;

  NS::SharedPtr<MTL::ResidencySet> main_residency_set_;

  size_t frames_in_flight_{rhi::k_max_frames_in_flight};
  size_t frame_num_{};
};

} // namespace gfx::metal
