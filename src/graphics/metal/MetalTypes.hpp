#pragma once

// clang-format off
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <Metal/MTLDrawable.hpp>
#include <Metal/MTLTexture.hpp>

namespace CA {
class MetalLayer;
}

struct Swapchain_Metal {
  NS::SharedPtr<CA::MetalLayer> layer;
  NS::SharedPtr<MTL::Drawable> curr_drawable;
  NS::SharedPtr<MTL::Texture> curr_texture;
};

namespace gfx::rhi {
struct Swapchain;
}

template <typename T>
struct MetalType;

template <>
struct MetalType<gfx::rhi::Swapchain> {
  using type = Swapchain_Metal;
};

template <typename T>
typename MetalType<T>::type* to_internal(const T& param) {
  return static_cast<typename MetalType<T>::type*>(param.internal_data.get());
}
