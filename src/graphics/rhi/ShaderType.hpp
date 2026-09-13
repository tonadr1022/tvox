#pragma once

#include <cstdint>

namespace gfx::rhi {

enum class ShaderType : uint8_t { None, Vertex, Fragment, Task, Mesh, Compute };

}  // namespace gfx::rhi
