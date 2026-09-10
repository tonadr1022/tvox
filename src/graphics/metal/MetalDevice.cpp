#include "MetalDevice.hpp"

#include <Foundation/NSSharedPtr.hpp>

#include "core/EAssert.hpp"
#include "core/Util.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"

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
  }
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

  frame_num_++;
}

}  // namespace gfx::metal
