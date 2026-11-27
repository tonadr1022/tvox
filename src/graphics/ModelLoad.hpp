#pragma once

#include "graphics/BindlessTexture.hpp"
#include "graphics/util/IndexAllocator.hpp"
#include "vertex_shared.h"
#include <filesystem>
#include <glm/vec3.hpp>
#include <vector>

namespace gfx {

namespace rhi {
class Device;
}

struct Model {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<BindlessTexture> textures;
};

class UploadQueue;

Model load_model(rhi::Device& device, UploadQueue& upload_queue,
                 IndexAllocator& texture_descriptor_index_allocator,
                 const std::filesystem::path& path);

} // namespace gfx
