#pragma once

#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/util/BumpAllocator.hpp"

namespace gfx {

class UploadQueue {
public:
  void init(rhi::Device* device, uint64_t max_size, uint64_t max_pending_batches);

  void upload_to_buffer(rhi::Buffer& buffer, size_t dst_offset, std::span<std::byte> data);
  rhi::TimelinePoint flush();

private:
  rhi::Device* device_;

  struct StagingBatch {
    rhi::Buffer buffer;
    BumpAllocator allocator;
    rhi::TimelinePoint timeline;
  };

  rhi::CmdPool cmd_pool_;
  std::vector<StagingBatch> staging_batches_;
  std::vector<StagingBatch> full_batches_;
};

} // namespace gfx
