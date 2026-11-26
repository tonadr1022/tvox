#pragma once

// clang-format off
#include <Metal/MTLResidencySet.hpp>
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include "graphics/metal/MetalCmdEncoder.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

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
  bool create_shader(rhi::ShaderType type, const void* data, uint64_t size, rhi::Shader& shader,
                     std::string_view entry_point) override;
  bool create_texture(rhi::Texture& tex, const rhi::TextureDesc& desc) override;
  void destroy_texture(rhi::Texture& texture) override;
  bool create_buffer(const rhi::BufferDesc& desc, rhi::Buffer& buffer) override;
  void destroy_buffer(rhi::Buffer& buffer) override;
  bool create_cmd_pool(rhi::CmdPool& pool) override;
  void reset_cmd_pool(rhi::CmdPool& pool) override;
  void write_texture_desc(const rhi::TextureDescriptorDesc& desc, rhi::Texture& texture,
                          uint32_t index) override;
  void create_timeline_semaphore(rhi::TimelineSemaphore& sema) override;
  void wait_timeline_semaphore(rhi::TimelineSemaphore& sema, uint64_t value) override;

  rhi::CmdEncoder* begin_cmd_encoder(rhi::CmdPool& pool) override;
  void end_cmd_encoder(rhi::CmdEncoder* encoder) override;

  MTL::Device* get_device() const {
    return device_.get();
  }

  void submit(std::span<rhi::CmdEncoder*> cmds, rhi::TimelinePoint& timeline_point) override;

  MTL::RenderPipelineState*
  get_or_create_render_pipeline(rhi::Pipeline& pipeline,
                                const rhi::RenderPassInfo& renderpass_info);

private:
  struct PipelineHashHasher {
    size_t operator()(const rhi::PipelineHash& hash) const {
      return static_cast<size_t>(hash.get_hash());
    }
  };
  void make_resident(MTL::Allocation* alloc);
  void remove_resident(MTL::Allocation* alloc);

  NS::SharedPtr<MTL::Device> device_;
  NS::SharedPtr<MTL4::CommandQueue> queue_;

  std::unordered_map<rhi::PipelineHash, NS::SharedPtr<MTL::RenderPipelineState>, PipelineHashHasher>
      pipelines_cache_;

  NS::SharedPtr<MTL::ResidencySet> main_residency_set_;

  struct TextureDescriptorHeap {
    NS::SharedPtr<MTL::TextureViewPool> tex_view_pool;
    NS::SharedPtr<MTL::Buffer> base_id_buf;
  } tex_desc_heap_;
  constexpr static auto k_descriptor_heap_arg_table_index{1};

  size_t frames_in_flight_{rhi::k_max_frames_in_flight};
};

} // namespace gfx::metal
