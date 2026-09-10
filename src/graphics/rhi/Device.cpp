#include "Device.hpp"

#include "core/Logger.hpp"

#ifdef __APPLE__
#include "graphics/metal/MetalDevice.hpp"
#endif

namespace gfx {

rhi::Device make_device() {
#ifdef __APPLE__
  return metal::MetalDevice{};
#else
  LCRITICAL("Unsupported OS");
#endif
}

} // namespace gfx
