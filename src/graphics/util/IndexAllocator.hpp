#pragma once

#include "core/Assert.hpp"
#include <cstdint>
#include <vector>

namespace gfx {

struct IndexAllocator {
  void init(uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
      free_indices.push_back(count - 1 - i);
    }
  }

  uint32_t alloc() {
    ASSERT(!free_indices.empty());
    auto index = free_indices.back();
    free_indices.pop_back();
    return index;
  }

  void free(uint32_t index) {
    free_indices.push_back(index);
  }

  std::vector<uint32_t> free_indices;
};

} // namespace gfx
