#pragma once

#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"
#include "graphics/util/BumpAllocator.hpp"
#include <glm/vec3.hpp>

namespace gfx {

struct TextureUploadDesc {
  glm::uvec3 src_size{};
  void* data{};
  uint32_t array_layers{}, mip_count{};
  uint32_t bytes_per_row{};
};

class UploadQueue {
public:
  void init(rhi::Device* device, uint64_t max_size, uint64_t max_pending_batches);

  void upload_to_buffer(rhi::Buffer& buffer, uint32_t dst_offset, std::span<std::byte> data);
  void upload_to_texture(rhi::Texture& texture, const TextureUploadDesc& desc);
  rhi::TimelinePoint flush();

private:
  rhi::Device* device_;

  struct StagingBatch {
    rhi::Buffer buffer;
    BumpAllocator allocator;
  };

  rhi::TimelinePoint timeline_;
  rhi::CmdPool cmd_pool_;
  std::vector<StagingBatch> staging_batches_;
  std::vector<StagingBatch> full_batches_;
  std::vector<rhi::CmdEncoder*> cmd_encoders_;
};

} // namespace gfx
