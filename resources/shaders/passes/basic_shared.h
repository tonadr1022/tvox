#pragma once

struct BasicRoot {
  float* output_result;
  float color_x;
  float padding0;
};

#if !defined(__SLANG__)
static_assert(sizeof(BasicRoot) == 16);
#endif
