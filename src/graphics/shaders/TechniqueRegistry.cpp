#include "graphics/shaders/TechniqueRegistry.hpp"
#include "graphics/rhi/ShaderType.hpp"

namespace gfx {
namespace {

constexpr ShaderTechniqueDesc::StageDesc k_basic_stages[] = {
    {.stage = rhi::ShaderType::Vertex, .entry = "vs_main"},
    {.stage = rhi::ShaderType::Fragment, .entry = "fs_main"},
};

constexpr ShaderTechniqueDesc::StageDesc k_comp_stage[] = {
    {.stage = rhi::ShaderType::Compute, .entry = "comp_main"},
};

constexpr ShaderTechniqueDesc k_techniques[] = {
    {
        .name = "basic_tri",
        .path = "passes/basic_tri",
        .stages = k_basic_stages,
    },
    {
        .name = "basic",
        .path = "passes/basic",
        .stages = k_comp_stage,
    },
    {
        .name = "post_process",
        .path = "passes/post_process",
        .stages = k_basic_stages,
    },
};

} // namespace

std::span<const ShaderTechniqueDesc> ShaderTechniqueRegistry::all() {
  return k_techniques;
}

const ShaderTechniqueDesc* ShaderTechniqueRegistry::find(std::string_view name) {
  for (const ShaderTechniqueDesc& tech : k_techniques) {
    if (tech.name == name) {
      return &tech;
    }
  }
  return nullptr;
}

} // namespace gfx
