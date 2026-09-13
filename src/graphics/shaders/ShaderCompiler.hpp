#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace gfx::shaders {

enum class ShaderStage : uint8_t { Vertex, Fragment, Task, Mesh, Compute };

// Vulkan/Spirv not supported yet
struct CompileInput {
  std::filesystem::path source_path;
  ShaderStage stage{ShaderStage::Vertex};
  std::string entry_point{"main"};
  std::vector<std::string> include_directories;
  std::vector<std::string> defines;
  bool disable_optimization{false};
  bool embed_debug{false};
};

// Vulkan/Spirv not supported yet
struct CompileOutput {
  std::vector<uint8_t> dxil;
  std::vector<uint8_t> metallib;
  std::vector<std::string> dependencies;
  std::string error_message;
  std::string dxc_version;
  std::string metal_ir_converter_version;

  [[nodiscard]] bool ok() const { return error_message.empty() && !metallib.empty(); }
};

/// Extra directories searched (in order) before the executable dir and the
/// CMake-provided `third_party/shader_libs/bin` path.
void set_library_search_paths(std::vector<std::filesystem::path> paths);

/// In-process HLSL → DXIL (DXC) → metallib (Metal Shader Converter).
/// On failure, returns false and fills `out.error_message`.
[[nodiscard]] bool compile(const CompileInput& input, CompileOutput& out);

}  // namespace gfx::shaders
