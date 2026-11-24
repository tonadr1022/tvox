#pragma once

#include "core/Assert.hpp"
#include "graphics/rhi/Graphics.hpp"
#include <cstddef>

namespace gfx {

class BumpAllocator {
public:
  explicit BumpAllocator(rhi::GpuCpuRange<std::byte> range) : range_(range) {
  }

  template <typename T>
  rhi::GpuCpuRange<T> allocate(size_t size) {
    size_t required_size = size * sizeof(T);
    ASSERT(required_size <= range_.size - offset_);
    rhi::GpuCpuRange<T> range{
        .cpu = reinterpret_cast<T*>(range_.cpu + reinterpret_cast<std::uintptr_t>(offset_)),
        .gpu = reinterpret_cast<T*>(range_.gpu + reinterpret_cast<std::uintptr_t>(offset_)),
        .size = required_size,
    };
    offset_ += required_size;
    return range;
  }

  void reset() {
    offset_ = 0;
  }

private:
  rhi::GpuCpuRange<std::byte> range_;
  size_t offset_{};
};

} // namespace gfx