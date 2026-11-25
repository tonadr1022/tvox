#pragma once

struct PostProcessRoot {
  float mult;
  float padding0;
  float padding1;
  float padding2;
};

#ifndef __SLANG__
static_assert(sizeof(PostProcessRoot) == 16);
#endif
