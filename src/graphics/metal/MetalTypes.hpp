#pragma once

// clang-format off
#include "graphics/rhi/Graphics.hpp"
#include <Metal/MTL4CommandAllocator.hpp>
#include <Metal/MTLEvent.hpp>
#include <deque>
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

namespace gfx::metal {

class MetalCmdEncoder;
// TODO: why is this not in a namespace
struct SwapchainMetal {
  NS::SharedPtr<CA::MetalLayer> layer;
  NS::SharedPtr<MTL::Texture> curr_texture;
};

struct ShaderMetal {
  NS::SharedPtr<MTL::Library> library;
  NS::SharedPtr<MTL::Function> function;
};

struct PipelineMetal {
  NS::SharedPtr<MTL::RenderPipelineState> render_pipeline;
  NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;
};

struct GpuHeapMetal {
  NS::SharedPtr<MTL::Heap> heap;
  NS::SharedPtr<MTL::Buffer> buffer;
};

struct BufferMetal {
  NS::SharedPtr<MTL::Buffer> buffer;
};

struct CmdPoolMetal {
  std::deque<std::unique_ptr<MetalCmdEncoder>> encoders;
  std::deque<std::unique_ptr<MetalCmdEncoder>> free_encoders;
  NS::SharedPtr<MTL4::CommandAllocator> cmd_allocator;
};

struct TextureMetal {
  NS::SharedPtr<MTL::Texture> texture;
};

struct TimelineSemaphoreMetal {
  NS::SharedPtr<MTL::SharedEvent> event;
};

template <typename T>
struct MetalType;

#define MAKE_METAL_TYPE(name)                                                                      \
  template <>                                                                                      \
  struct MetalType<gfx::rhi::name> {                                                               \
    using type = name##Metal;                                                                      \
  };

MAKE_METAL_TYPE(Swapchain)
MAKE_METAL_TYPE(Shader)
MAKE_METAL_TYPE(Pipeline)
MAKE_METAL_TYPE(GpuHeap)
MAKE_METAL_TYPE(Buffer)
MAKE_METAL_TYPE(CmdPool)
MAKE_METAL_TYPE(Texture)
MAKE_METAL_TYPE(TimelineSemaphore)

#undef MAKE_METAL_TYPE

template <typename T>
MetalType<T>::type* to_internal(const T& param) {
  return static_cast<MetalType<T>::type*>(param.internal_data.get());
}

template <typename T>
MetalType<T>::type* to_internal(const T* param) {
  ASSERT(param);
  return static_cast<MetalType<T>::type*>(param->internal_data.get());
}

} // namespace gfx::metal
