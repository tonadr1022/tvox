add_subdirectory(SDL)
add_subdirectory(metal-cpp)
add_subdirectory(glm)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
add_subdirectory(json)

add_subdirectory(shader_libs)
