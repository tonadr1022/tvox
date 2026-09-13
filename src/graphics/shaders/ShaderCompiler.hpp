#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace gfx {

enum class ShaderStage : uint8_t { Vertex, Fragment, Task, Mesh, Compute };

// Vulkan/Spirv not supported yet
struct ShaderCompileInput {
  std::filesystem::path source_path;
  ShaderStage stage{ShaderStage::Vertex};
  std::string entry_point{"main"};
  std::vector<std::string> include_directories;
  std::vector<std::string> defines;
  bool disable_optimization{false};
  bool embed_debug{false};
};

// Vulkan/Spirv not supported yet
struct ShaderCompileOutput {
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

struct ShaderToolVersions {
  std::string dxc;
  std::string metal_ir_converter;
  std::string error_message;

  [[nodiscard]] bool ok() const { return error_message.empty(); }
};

/// Load DXC + Metal IR Converter (if needed) and return version strings for cache keys.
[[nodiscard]] ShaderToolVersions query_shader_tool_versions();

/// In-process HLSL → DXIL (DXC) → metallib (Metal Shader Converter).
/// On failure, returns false and fills `out.error_message`.
[[nodiscard]] bool compile(const ShaderCompileInput& input, ShaderCompileOutput& out);

}  // namespace gfx
