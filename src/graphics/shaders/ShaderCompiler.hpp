#pragma once

#include "graphics/rhi/ShaderType.hpp"

#include <slang-com-ptr.h>
#include <slang.h>

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace gfx {

struct ShaderCompileInput {
  std::filesystem::path source_path;
  rhi::ShaderType stage{rhi::ShaderType::Vertex};
  std::string entry_point{"main"};
  std::vector<std::string> include_directories;
  std::vector<std::string> defines;
  bool disable_optimization{false};
  bool embed_debug{false};
};

struct ShaderCompileOutput {
  std::vector<std::byte> bytes;
  std::vector<std::string> dependency_paths;
  std::string slang_version;

  [[nodiscard]]
  bool ok() const {
    return !bytes.empty();
  }
};

struct ShaderToolVersions {
  std::string slang_version;
  std::string error_message;

  bool ok() const {
    return error_message.empty() && !slang_version.empty();
  }
};

class ShaderCompiler {
public:
  void init() {
  }
  std::expected<ShaderCompileOutput, std::string> compile(const ShaderCompileInput& compile_input);

private:
  void init_if_needed();
  Slang::ComPtr<slang::IGlobalSession> global_session_;
  Slang::ComPtr<slang::ISession> session_;

  bool initialized_global_session_{};
};

// TODO: move into class. move everything here into the class.
[[nodiscard]]
ShaderToolVersions query_shader_tool_versions();

} // namespace gfx
