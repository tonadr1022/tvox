#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "graphics/shaders/ShaderCompiler.hpp"

namespace gfx::shaders {

/// DXIL → metallib via Apple Metal Shader Converter.
/// `library_dirs` is used on the first successful load of libmetalirconverter.
[[nodiscard]] bool convert_dxil_to_metallib(const CompileInput& input,
                                            const std::vector<uint8_t>& dxil,
                                            const std::vector<std::filesystem::path>& library_dirs,
                                            CompileOutput& out);

}  // namespace gfx::shaders
