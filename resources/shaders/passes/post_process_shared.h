#pragma once

#include "shader_defines.h" // IWYU pragma: keep

struct PostProcessRoot {
  float mult;
  uint32_t tex;
  float padding1;
  float padding2;
};

#ifndef __SLANG__
static_assert(sizeof(PostProcessRoot) == 16);
#endif
