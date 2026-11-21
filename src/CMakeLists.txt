set(target_name tvox)

add_library(tvox_shader_compiler STATIC
  core/Paths.cpp
  core/DynamicLib.cpp
  core/FileIo.cpp
  graphics/shaders/ShaderCompiler.cpp
  graphics/shaders/TechniqueRegistry.cpp
  graphics/shaders/ShaderCache.cpp
)
target_include_directories(tvox_shader_compiler PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(tvox_shader_compiler PUBLIC
  slang
  SDL3::SDL3
  nlohmann_json::nlohmann_json
)

add_executable(${target_name}
  main.cpp
  App.cpp
  platform/Window.cpp
  graphics/Renderer.cpp
  graphics/rhi/Device.cpp
)

set(BUILD_METAL ON)

if (${BUILD_METAL})
    target_sources(${target_name} PRIVATE
    platform/apple/AppleWindow.mm
    graphics/metal/MetalDevice.cpp
    graphics/metal/MetalCmdEncoder.cpp
    graphics/metal/MetalWindow.mm
    graphics/metal/MetalCpp.cpp
  )
endif()

target_link_libraries(${target_name} PRIVATE
  slang
  METAL_CPP
  SDL3::SDL3
  glm::glm
  tvox_shader_compiler
)

target_include_directories(${target_name} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/vendored
)