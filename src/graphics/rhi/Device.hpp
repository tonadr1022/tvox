#pragma once

#include "graphics/rhi/Graphics.hpp"

#include <memory>
#include <string_view>

struct SDL_Window;

namespace gfx::rhi {

class CmdEncoder;
class Swapchain;
struct SwapchainDesc;

constexpr int k_max_frames_in_flight = 2;

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

  // empty entry_point -> "main"
  virtual bool create_shader(rhi::ShaderType type, const void* data, size_t size,
                             rhi::Shader& shader, std::string_view entry_point) = 0;

  virtual void submit_queue() = 0;
  virtual rhi::CmdEncoder* begin_cmd_encoder() = 0;
  virtual void end_cmd_encoder(rhi::CmdEncoder* encoder) = 0;
};

} // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
