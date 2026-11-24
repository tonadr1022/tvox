#pragma once

#include "shader_defines.h"

struct Vertex {
  float3 position;
};

struct ForwardRoot {
  float* output_result;
  Vertex* vertices;
  float padding0;
};

#ifndef __SLANG__
static_assert(sizeof(ForwardRoot) == 24);
#endif
