#pragma once

#include <span>
#include <string_view>

#include "graphics/rhi/ShaderType.hpp"

namespace gfx {

/// One cookable technique. Permutations are separate rows
struct ShaderTechniqueDesc {
  struct StageDesc {
    rhi::ShaderType stage{rhi::ShaderType::Vertex};
    std::string_view entry{"main"};
  };

  std::string_view name;
  /// Path relative to the shader root (e.g. `passes/basic.hlsl`).
  std::string_view path;
  std::span<const StageDesc> stages;
  std::span<const std::string_view> defines;
};

class ShaderTechniqueRegistry {
 public:
  [[nodiscard]] static std::span<const ShaderTechniqueDesc> all();
  [[nodiscard]] static const ShaderTechniqueDesc* find(std::string_view name);
};

}  // namespace gfx
