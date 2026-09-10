#pragma once

namespace gfx::rhi {

class Device {
 public:
  ~Device();
  virtual void init() = 0;
};

}  // namespace gfx::rhi

namespace gfx {

rhi::Device make_device();

}
