#pragma once

#include "graphics/shaders/ShaderCompiler.hpp"

#include <filesystem>
#include <vector>

namespace gfx {

/// DXIL → metallib via Apple Metal Shader Converter.
/// `library_dirs` is used on the first successful load of libmetalirconverter.
[[nodiscard]] bool convert_dxil_to_metallib(const ShaderCompileInput& input,
                                            const std::vector<std::byte>& dxil,
                                            const std::vector<std::filesystem::path>& library_dirs,
                                            ShaderCompileOutput& out);

[[nodiscard]] bool query_metal_ir_version(const std::vector<std::filesystem::path>& library_dirs,
                                          std::string& version, std::string& error);

} // namespace gfx
