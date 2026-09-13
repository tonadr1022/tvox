#include "graphics/shaders/TechniqueRegistry.hpp"

namespace gfx {
namespace {

constexpr ShaderTechniqueDesc::StageDesc k_basic_stages[] = {
    {.stage = rhi::ShaderType::Vertex, .entry = "vs_main"},
    {.stage = rhi::ShaderType::Fragment, .entry = "fs_main"},
};

constexpr ShaderTechniqueDesc k_techniques[] = {
    {
        .name = "basic",
        .path = "passes/basic.hlsl",
        .stages = k_basic_stages,
    },
};

}  // namespace

std::span<const ShaderTechniqueDesc> ShaderTechniqueRegistry::all() { return k_techniques; }

const ShaderTechniqueDesc* ShaderTechniqueRegistry::find(std::string_view name) {
  for (const ShaderTechniqueDesc& tech : k_techniques) {
    if (tech.name == name) {
      return &tech;
    }
  }
  return nullptr;
}

}  // namespace gfx
