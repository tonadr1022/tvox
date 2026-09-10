#include "Device.hpp"

#include "core/Logger.hpp"  // IWYU pragma: keep

#ifdef __APPLE__
#include "graphics/metal/MetalDevice.hpp"
#endif

namespace gfx {

std::unique_ptr<rhi::Device> make_device() {
#ifdef __APPLE__
  return std::make_unique<metal::MetalDevice>();
#else
  LCRITICAL("Unsupported OS");
  std::exit(1);
#endif
}

}  // namespace gfx
