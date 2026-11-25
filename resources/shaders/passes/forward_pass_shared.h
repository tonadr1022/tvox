#pragma once

#include "vertex_shared.h"

struct ForwardRoot {
  float4x4 view_proj;
  float* output_result;
  Vertex* vertices;
  float padding0;
};

#ifndef __SLANG__
static_assert(sizeof(ForwardRoot) == 88);
#endif
