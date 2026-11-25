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
  virtual bool create_cmd_pool(rhi::CmdPool& pool) = 0;
  virtual void reset_cmd_pool(rhi::CmdPool& pool) = 0;
  // empty entry_point -> "main"
  virtual bool create_shader(rhi::ShaderType type, const void* data, size_t size,
                             rhi::Shader& shader, std::string_view entry_point) = 0;
  virtual void submit(std::span<rhi::CmdEncoder*> cmds) = 0;
  virtual rhi::CmdEncoder* begin_cmd_encoder(rhi::CmdPool& pool) = 0;
  virtual void end_cmd_encoder(rhi::CmdEncoder* encoder) = 0;
};

} // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
