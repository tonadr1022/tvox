#pragma once

#if !defined(__SLANG__)

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

using float3 = glm::vec3;
using float4x4 = glm::mat4;

#define GPU_ROOT(type, var_name)

#else

#define GPU_ROOT(type, var_name)                                                                   \
  [[vk::binding(0)]]                                                                               \
  ConstantBuffer<type> var_name : register(b0);

#endif
