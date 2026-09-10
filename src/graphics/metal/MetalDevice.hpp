#pragma once

// clang-format off
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <Metal/Metal.hpp>

#include "graphics/rhi/Device.hpp"

namespace MTL {
class Device;
}

namespace gfx::metal {

class MetalDevice : public gfx::rhi::Device {
 public:
  ~MetalDevice() override;
  void init() override;

 private:
  NS::SharedPtr<MTL::Device> device_;
};

}  // namespace gfx::metal
