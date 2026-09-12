#pragma once

#include <memory>

namespace gfx::rhi {

class CmdEncoder;
class Swapchain;
struct SwapchainDesc;

constexpr int k_max_frames_in_flight = 3;

class Device {
 public:
  virtual ~Device();

  virtual void init() = 0;

  virtual void create_swapchain(const rhi::SwapchainDesc& desc, rhi::Swapchain& swapchain) = 0;

  virtual void submit_queue() = 0;
  virtual rhi::CmdEncoder* begin_cmd_encoder() = 0;
  virtual void end_cmd_encoder(rhi::CmdEncoder* encoder) = 0;
};

}  // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
