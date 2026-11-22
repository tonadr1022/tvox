#pragma once

#include "core/Assert.hpp"
#include <cstddef>

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

inline size_t align_up(size_t value, size_t alignment) {
  ASSERT((alignment & (alignment - 1)) == 0);
  return (value + (alignment - 1)) & ~(alignment - 1);
}
