#include "UploadQueue.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/util/BumpAllocator.hpp"

namespace gfx {

void UploadQueue::init(rhi::Device* device, uint64_t max_size, uint64_t max_pending_batches) {
  device_ = device;
  device_->create_cmd_pool(cmd_pool_, 1);

  for (uint64_t i = 0; i < max_pending_batches; i++) {
    rhi::Buffer buf{};
    device_->create_buffer({.size = max_size, .memory_type = rhi::MemoryType::CpuVisible}, buf);
    auto range = rhi::gpu_cpu_range(buf);
    auto& batch = staging_batches_.emplace_back(std::move(buf), BumpAllocator{range});
  }
  device_->create_timeline_semaphore(timeline_.sema);
}

void UploadQueue::upload_to_buffer(rhi::Buffer& buffer, size_t dst_offset,
                                   std::span<std::byte> data) {
  // TODO: find a free batch
  StagingBatch* batch{};
  for (auto& b : staging_batches_) {
    size_t remaining_bytes = b.allocator.remaining_bytes();
    if (data.size_bytes() <= remaining_bytes) {
      batch = &b;
      break;
    }
  }

  ASSERT(batch);

  auto* enc = device_->begin_cmd_encoder(cmd_pool_);
  auto curr_offset = batch->allocator.curr_offset();
  auto alloc = batch->allocator.allocate<std::byte>(data.size());
  memcpy(alloc.cpu, data.data(), data.size());
  enc->copy_buffer_to_buffer(buffer, dst_offset, batch->buffer, curr_offset, data.size());
  device_->end_cmd_encoder(enc);
  cmd_encoders_.push_back(enc);
}

rhi::TimelinePoint UploadQueue::flush() {
  timeline_.value++;
  device_->submit(cmd_encoders_, {}, timeline_);
  cmd_encoders_.clear();
  return timeline_;
}

} // namespace gfx
