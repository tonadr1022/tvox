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
    ASSERT(offset_ <= range_.size);
    auto padding = get_padding();
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

  size_t curr_offset() const {
    return offset_;
  }

  size_t remaining_bytes() const {
    auto padding = get_padding();
    return range_.size - offset_ + padding;
  }

  void reset() {
    offset_ = 0;
  }

private:
  size_t get_padding() const {
    const auto address = reinterpret_cast<std::uintptr_t>(range_.gpu) + offset_;
    constexpr size_t alignment = 16;
    return align_up(address, alignment) - address;
  }
  rhi::GpuCpuRange<std::byte> range_;
  size_t offset_{};
};

} // namespace gfx
