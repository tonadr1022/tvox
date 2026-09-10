#include "Renderer.hpp"

#include "graphics/rhi/Device.hpp"
namespace gfx {

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::init() {
  device_ = make_device();
  device_->init();
}

void Renderer::render() {
  auto* enc = device_->begin_cmd_encoder();
  device_->end_cmd_encoder(enc);
  device_->submit_queue();
}

}  // namespace gfx
