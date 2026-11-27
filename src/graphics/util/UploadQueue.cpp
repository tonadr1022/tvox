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

void UploadQueue::upload_to_texture(rhi::Texture& texture, const TextureUploadDesc& desc) {
  uint32_t copy_size = desc.bytes_per_row * desc.src_size.y * desc.src_size.z;
  StagingBatch* batch{};
  for (auto& b : staging_batches_) {
    size_t remaining_bytes = b.allocator.remaining_bytes();
    // TODO: calculate total copy size
    if (copy_size <= remaining_bytes) {
      batch = &b;
      break;
    }
  }

  ASSERT(batch);

  auto* enc = device_->begin_cmd_encoder(cmd_pool_);

  const auto* src_data = static_cast<const std::byte*>(desc.data);
  for (uint32_t y = 0; y < desc.src_size.y; y++) {
    // copy each row to the dst texture
    auto curr_offset = batch->allocator.curr_offset();
    auto alloc = batch->allocator.allocate<std::byte>(desc.bytes_per_row);
    memcpy(alloc.cpu, src_data + static_cast<size_t>(y) * desc.bytes_per_row, desc.bytes_per_row);
    enc->copy_texture_from_buffer(texture, 0, 0, batch->buffer, curr_offset, desc.bytes_per_row,
                                  glm::uvec3{desc.src_size.x, 1, 1}, glm::uvec3{0, y, 0});
  }

  device_->end_cmd_encoder(enc);

  cmd_encoders_.push_back(enc);
}

void UploadQueue::upload_to_buffer(rhi::Buffer& buffer, uint32_t dst_offset,
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
