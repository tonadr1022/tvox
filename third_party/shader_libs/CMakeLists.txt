# Shader compile libraries (DXC + Apple Metal Shader Converter)
#
# Headers are vendored under include/. Runtime dylibs live in bin/ and are
# gitignored (large Apple/Microsoft binaries).
#
# Populate bin/ (once per machine):
#   cp /path/to/libdxcompiler.dylib third_party/shader_libs/bin/
#   cp /path/to/libmetalirconverter.dylib third_party/shader_libs/bin/
#
# Known sources used while bootstrapping tvox:
#   - libdxcompiler.dylib: WickedEngine / DirectXShaderCompiler macOS builds
#   - libmetalirconverter.dylib + headers: Apple Metal Shader Converter SDK
#     (also mirrored in WickedEngine Utility/metal and Kaleidoscope thirdparty)
#
# At runtime the compiler searches, in order:
#   1. paths from gfx::shaders::set_library_search_paths
#   2. directory of the running executable (SDL_GetBasePath)
#   3. this bin/ directory (compile-time absolute path)
#   4. current working directory
#   5. bare soname via the platform dynamic loader

cmake_minimum_required(VERSION 3.20)

set(TVOX_SHADER_LIBS_BIN "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH
    "Directory containing libdxcompiler.dylib and libmetalirconverter.dylib")

add_library(tvox_shader_libs INTERFACE)
target_include_directories(tvox_shader_libs INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include/dxc"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

function(tvox_copy_shader_libs target_name)
  set(_dxc "${TVOX_SHADER_LIBS_BIN}/libdxcompiler.dylib")
  set(_msc "${TVOX_SHADER_LIBS_BIN}/libmetalirconverter.dylib")
  if(NOT EXISTS "${_dxc}" OR NOT EXISTS "${_msc}")
    message(WARNING
      "Shader dylibs missing under ${TVOX_SHADER_LIBS_BIN}. "
      "See third_party/shader_libs/README.md. In-process compile will fail at runtime.")
    return()
  endif()
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_dxc}" "$<TARGET_FILE_DIR:${target_name}>/libdxcompiler.dylib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_msc}" "$<TARGET_FILE_DIR:${target_name}>/libmetalirconverter.dylib"
    COMMENT "Copy shader compiler dylibs next to ${target_name}"
  )
endfunction()
