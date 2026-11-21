#pragma once

#include <cstdint>

namespace gfx::rhi {

enum class ShaderType : uint8_t { None, Vertex, Fragment, Task, Mesh, Compute };

inline const char* to_string(ShaderType type) {
  switch (type) {
  case ShaderType::None:
    return "None";
  case ShaderType::Vertex:
    return "Vertex";
  case ShaderType::Fragment:
    return "Fragment";
  case ShaderType::Task:
    return "Task";
  case ShaderType::Mesh:
    return "Mesh";
  case ShaderType::Compute:
    return "Compute";
    break;
  }
}

} // namespace gfx::rhi
