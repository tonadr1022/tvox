#pragma once

// clang-format off
#include <Metal/MTLComputePipeline.hpp>
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <Metal/MTLDrawable.hpp>
#include <Metal/MTLLibrary.hpp>
#include <Metal/MTLTexture.hpp>

namespace CA {
class MetalLayer;
}

struct Swapchain_Metal {
  NS::SharedPtr<CA::MetalLayer> layer;
  NS::SharedPtr<MTL::Texture> curr_texture;
};

struct Shader_Metal {
  NS::SharedPtr<MTL::Library> library;
  NS::SharedPtr<MTL::Function> function;
  NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;
};

template <typename T>
struct MetalType;

namespace gfx::rhi {
struct Swapchain;
struct Shader;
}  // namespace gfx::rhi

template <>
struct MetalType<gfx::rhi::Swapchain> {
  using type = Swapchain_Metal;
};

template <>
struct MetalType<gfx::rhi::Shader> {
  using type = Shader_Metal;
};

template <typename T>
typename MetalType<T>::type* to_internal(const T& param) {
  return static_cast<typename MetalType<T>::type*>(param.internal_data.get());
}

template <typename T>
typename MetalType<T>::type* to_internal(const T* param) {
  ASSERT(param);
  return static_cast<typename MetalType<T>::type*>(param->internal_data.get());
}
