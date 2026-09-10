set(target_name tvox)

add_executable(${target_name}
main.cpp
App.cpp
platform/Window.cpp
)

set(BUILD_METAL ON)

if (${BUILD_METAL})
    target_sources(${target_name} PRIVATE
        platform/apple/AppleWindow.mm
        platform/apple/AppleWindow.hpp
        graphics/metal/MetalDevice.cpp
        graphics/metal/MetalDevice.hpp
    )
endif()

target_link_libraries(${target_name} PRIVATE
METAL_CPP
SDL3::SDL3
)

target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

