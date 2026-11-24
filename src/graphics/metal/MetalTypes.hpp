#pragma once

// clang-format off
#include "graphics/rhi/Graphics.hpp"
#include <type_traits> // IWYU pragma: keep
#include <Foundation/NSSharedPtr.hpp>
// clang-format on

#include <Metal/MTLComputePipeline.hpp>
#include <Metal/MTLDrawable.hpp>
#include <Metal/MTLLibrary.hpp>
#include <Metal/MTLRenderPipeline.hpp>
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
};

struct Pipeline_Metal {
  NS::SharedPtr<MTL::RenderPipelineState> render_pipeline;
  NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;
};

struct GpuHeap_Metal {
  NS::SharedPtr<MTL::Heap> heap;
  NS::SharedPtr<MTL::Buffer> buffer;
};

struct Buffer_Metal {
  NS::SharedPtr<MTL::Buffer> buffer;
};

template <typename T>
struct MetalType;

namespace gfx::rhi {
struct Swapchain;
struct Shader;
struct Pipeline;
} // namespace gfx::rhi

#define MAKE_METAL_TYPE(name)                                                                      \
  template <>                                                                                      \
  struct MetalType<gfx::rhi::name> {                                                               \
    using type = name##_Metal;                                                                     \
  };

MAKE_METAL_TYPE(Swapchain)
MAKE_METAL_TYPE(Shader)
MAKE_METAL_TYPE(Pipeline)
MAKE_METAL_TYPE(GpuHeap)
MAKE_METAL_TYPE(Buffer)

#undef MAKE_METAL_TYPE

template <typename T>
typename MetalType<T>::type* to_internal(const T& param) {
  return static_cast<typename MetalType<T>::type*>(param.internal_data.get());
}

template <typename T>
typename MetalType<T>::type* to_internal(const T* param) {
  ASSERT(param);
  return static_cast<typename MetalType<T>::type*>(param->internal_data.get());
}
