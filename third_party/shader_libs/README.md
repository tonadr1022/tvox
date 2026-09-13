# Shader compile libraries (DXC + Apple Metal Shader Converter)
#
# Headers are vendored under `include/`. Runtime dylibs live in `bin/` and are
# gitignored (large binaries).
#
# Populate `bin/` once:
#
# ```sh
# cp /path/to/libdxcompiler.dylib third_party/shader_libs/bin/
# cp /path/to/libmetalirconverter.dylib third_party/shader_libs/bin/
# ```
#
# Bootstrapping sources used for tvox:
# - `libdxcompiler.dylib`: DirectXShaderCompiler / WickedEngine macOS build
# - `libmetalirconverter.dylib` + headers: Apple Metal Shader Converter SDK
#
# Runtime search order (see `gfx::shaders::compile`):
# 1. `set_library_search_paths(...)`
# 2. directory of the running executable
# 3. this `bin/` directory (absolute path baked at configure time)
# 4. current working directory
# 5. bare soname via dyld
