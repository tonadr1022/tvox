#include "MetalDevice.hpp"

#include "core/EAssert.hpp"

namespace gfx::metal {

void MetalDevice::init() {
  device_ = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
  ASSERT(device_);
}

MetalDevice::~MetalDevice() = default;

}  // namespace gfx::metal
