#include "MetalDevice.hpp"

#include <Foundation/NSSharedPtr.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include "core/EAssert.hpp"
#include "core/Util.hpp"
#include "graphics/metal/MetalTypes.hpp"
#include "graphics/metal/MetalWindow.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

namespace gfx::metal {

MetalDevice::~MetalDevice() = default;

void MetalDevice::init() {
  device_ = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

  frames_in_flight_ = rhi::k_max_frames_in_flight;

  queue_ = NS::TransferPtr(device_->newMTL4CommandQueue());

  for (int i = 0; i < frames_in_flight_; i++) {
    auto& frame = per_frame_[i];
    frame.cmd_allocator = NS::TransferPtr(device_->newCommandAllocator());
    frame.cmd_buf = NS::TransferPtr(device_->newCommandBuffer());
    frame.cmd_encoder.emplace();
    frame.cmd_encoder->set_cmd_buffer(frame.cmd_buf);
    frame.fence = NS::TransferPtr(device_->newSharedEvent());
  }
}

void MetalDevice::create_swapchain(const rhi::SwapchainDesc& desc, rhi::Swapchain& swapchain) {
  // TODO: reset existing swapchain state
  ASSERT(!swapchain.internal_data);
  swapchain.internal_data = wi::allocator::make_shared<Swapchain_Metal>();
  auto* internal_data = to_internal(swapchain);

  if (!internal_data->layer) {
    internal_data->layer = NS::TransferPtr(CA::MetalLayer::layer());
    auto* layer = internal_data->layer.get();
    layer = internal_data->layer.get();
    layer->setDevice(device_.get());
    layer->setDisplaySyncEnabled(true);
    layer->setMaximumDrawableCount(3);
  }

  auto* layer = internal_data->layer.get();
  layer->setDrawableSize(CGSize{.width = static_cast<CGFloat>(desc.width),
                                .height = static_cast<CGFloat>(desc.height)});

  set_layer_for_window(swapchain.desc.window, layer);
}

rhi::CmdEncoder* MetalDevice::begin_cmd_encoder() {
  auto& frame = curr_frame();
  frame.cmd_buf->beginCommandBuffer(frame.cmd_allocator.get());
  frame.cmd_buffers_allocated++;
  return &frame.cmd_encoder.value();
}

void MetalDevice::end_cmd_encoder([[maybe_unused]] rhi::CmdEncoder* encoder) {
  curr_frame().cmd_buf->endCommandBuffer();
}

void MetalDevice::submit_queue() {
  auto& frame = curr_frame();

  ASSERT(frame.cmd_buffers_allocated == 1);
  frame.cmd_buffers_allocated = 0;

  MTL4::CommandBuffer* submit_cmd_buffers[] = {
      frame.cmd_buf.get(),
  };

  queue_->commit(submit_cmd_buffers, ARRAY_SIZE(submit_cmd_buffers));

  // signal fence for the frame after work is complete
  frame.fence_value++;
  queue_->signalEvent(frame.fence.get(), frame.fence_value);

  // present submits now that frame is complete
  for (auto& present : frame.cmd_encoder->presents_) {
    present->present();
  }
  frame.cmd_encoder->presents_.clear();

  frame_num_++;

  // wait for the fence from FRAMES_IN_FLIGHT frames ago
  frame = curr_frame();
  frame.fence->waitUntilSignaledValue(frame.fence_value, UINT64_MAX);
}

}  // namespace gfx::metal
