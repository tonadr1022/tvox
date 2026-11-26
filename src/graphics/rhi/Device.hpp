#pragma once

#include "graphics/rhi/Graphics.hpp"
#include "graphics/rhi/ShaderType.hpp"

#include <memory>
#include <span>
#include <string_view>

struct SDL_Window;

namespace gfx::rhi {

class CmdEncoder;
class Swapchain;
struct SwapchainDesc;

class Device {
public:
  virtual ~Device();

  virtual void init() = 0;

  virtual void create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                                rhi::Swapchain& swapchain) = 0;
  virtual bool create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) = 0;
  virtual bool create_pipeline(const rhi::Shader& compute_shader, rhi::Pipeline& pipeline) = 0;
  virtual bool create_gpu_heap(const rhi::GpuHeapDesc& desc, rhi::GpuHeap& heap) = 0;
  virtual bool create_buffer(const rhi::BufferDesc& desc, rhi::Buffer& buffer) = 0;
  virtual void destroy_buffer(rhi::Buffer& buffer) = 0;
  virtual bool create_cmd_pool(rhi::CmdPool& pool) = 0;
  virtual void reset_cmd_pool(rhi::CmdPool& pool) = 0;
  virtual void write_texture_desc(const rhi::TextureDescriptorDesc& desc, rhi::Texture& texture,
                                  uint32_t index) = 0;
  virtual void create_timeline_semaphore(rhi::TimelineSemaphore& sema) = 0;
  virtual void wait_timeline_semaphore(rhi::TimelineSemaphore& sema, uint64_t value) = 0;
  // empty entry_point -> "main"
  virtual bool create_shader(rhi::ShaderType type, const void* data, uint64_t size,
                             rhi::Shader& shader, std::string_view entry_point) = 0;
  virtual bool create_texture(rhi::Texture& tex, const rhi::TextureDesc& desc) = 0;
  virtual void destroy_texture(rhi::Texture& texture) = 0;
  virtual void submit(std::span<rhi::CmdEncoder*> cmds, TimelinePoint& timeline_point) = 0;
  virtual rhi::CmdEncoder* begin_cmd_encoder(rhi::CmdPool& pool) = 0;
  virtual void end_cmd_encoder(rhi::CmdEncoder* encoder) = 0;
};

} // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
