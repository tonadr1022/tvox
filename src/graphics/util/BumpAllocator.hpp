#pragma once

#include "core/Assert.hpp"
#include "core/Util.hpp"
#include "graphics/rhi/Graphics.hpp"
#include <cstddef>
#include <cstdint>

namespace gfx {

class BumpAllocator {
public:
  explicit BumpAllocator(rhi::GpuCpuRange<std::byte> range) : range_(range) {
  }

  template <typename T>
  rhi::GpuCpuRange<T> allocate(size_t size) {
    constexpr size_t alignment = 16;
    ASSERT(offset_ <= range_.size);
    const auto address = reinterpret_cast<std::uintptr_t>(range_.gpu) + offset_;
    const size_t padding = align_up(address, alignment) - address;
    ASSERT(padding <= range_.size - offset_);
    offset_ += padding;

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
