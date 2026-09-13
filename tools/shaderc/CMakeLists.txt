add_executable(tvox-shaderc main.cpp)
target_link_libraries(tvox-shaderc PRIVATE
  tvox_shader_compiler
  SDL3::SDL3
  project_warnings
)
tvox_copy_shader_libs(tvox-shaderc)
