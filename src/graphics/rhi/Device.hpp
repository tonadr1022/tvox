#pragma once

#include <memory>

#include "graphics/rhi/Graphics.hpp"

struct SDL_Window;

namespace gfx::rhi {

class CmdEncoder;
class Swapchain;
struct SwapchainDesc;

constexpr int k_max_frames_in_flight = 3;

class Device {
 public:
  virtual ~Device();

  virtual void init() = 0;

  virtual void create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                                rhi::Swapchain& swapchain) = 0;
  virtual void create_pipeline(rhi::GraphicsPipelineCreateInfo& cinfo) = 0;
  virtual bool create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) = 0;
  virtual bool create_shader(rhi::ShaderType type, const void* data, size_t size,
                             rhi::Shader& shader) = 0;

  virtual void submit_queue() = 0;
  virtual rhi::CmdEncoder* begin_cmd_encoder() = 0;
  virtual void end_cmd_encoder(rhi::CmdEncoder* encoder) = 0;
};

}  // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
