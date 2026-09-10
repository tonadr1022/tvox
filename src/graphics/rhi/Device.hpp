#pragma once

#include <memory>

namespace gfx::rhi {

class Device {
 public:
  virtual ~Device() = default;
  virtual void init() = 0;
};

}  // namespace gfx::rhi

namespace gfx {

std::unique_ptr<rhi::Device> make_device();

}
