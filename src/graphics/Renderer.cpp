#include "Renderer.hpp"

#include <SDL3/SDL_video.h>

#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"

namespace gfx {

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::init(SDL_Window* window) {
  window_ = window;
  device_ = make_device();
  device_->init();
  {
    int w{}, h{};
    SDL_GetWindowSize(window_, &w, &h);
    device_->create_swapchain(rhi::SwapchainDesc{.window = window_,
                                                 .width = static_cast<uint32_t>(w),
                                                 .height = static_cast<uint32_t>(h)},
                              swapchain_);
  }
}

void Renderer::render() {
  auto* enc = device_->begin_cmd_encoder();
  enc->begin_rendering(swapchain_);
  enc->end_rendering();
  device_->end_cmd_encoder(enc);
  device_->submit_queue();
}

}  // namespace gfx
