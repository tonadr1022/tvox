#pragma once

#include "vertex_shared.h"
#include <filesystem>
#include <glm/vec3.hpp>
#include <vector>

namespace gfx {

struct Model {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

Model load_model(const std::filesystem::path& path);

} // namespace gfx
