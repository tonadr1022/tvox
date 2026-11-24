#pragma once

#include "graphics/rhi/ShaderType.hpp"
#include "small_vector/small_vector.hpp"
#include "wicked_engine/wiAllocator.h"

#include <cstdint>
#include <glm/vec4.hpp>

struct SDL_Window;

namespace gfx::rhi {

enum class Format : uint8_t {
  Unknown = 0,
  BGRA8_UNORM,
};

struct SwapchainDesc {
  uint32_t width, height;
  Format format{Format::BGRA8_UNORM};
};

struct Swapchain {
  wi::allocator::shared_ptr<void> internal_data;
  SwapchainDesc desc;

  [[nodiscard]]
  const SwapchainDesc& get_desc() const {
    return desc;
  }
};

struct Texture {
  wi::allocator::shared_ptr<void> internal_data;
};

struct ShaderDesc {
  std::string path;
};

struct Shader {
  wi::allocator::shared_ptr<void> internal_data;
  ShaderDesc desc;
};

struct PipelineDesc {
  Shader* vertex_shader{};
  Shader* mesh_shader{};
  Shader* task_shader{};
  Shader* fragment_shader{};
};

struct Pipeline {
  wi::allocator::shared_ptr<void> internal_data;
  PipelineDesc desc;
};

struct GpuRange {
  void* gpu{};
  size_t size{};
};

template <typename T>
struct GpuCpuRange {
  T* cpu{};
  T* gpu{};
  size_t size{};
};

template <typename T>
GpuRange gpu_range(const GpuCpuRange<T> range) {
  return {.gpu = range.gpu, .size = range.size};
}

template <typename T>
GpuCpuRange<T> gpu_range(const GpuCpuRange<T> range, size_t offset) {
  return {.cpu = range.cpu + offset, .gpu = range.gpu + offset, .size = range.size};
}

enum class MemoryType : uint8_t {
  CpuVisible,
  GpuOnly,
  Readback,
};

struct GpuHeapDesc {
  size_t size{};
  MemoryType memory_type{MemoryType::GpuOnly};
};

struct GpuHeap {
  wi::allocator::shared_ptr<void> internal_data;
  // TODO: For now this is cpu visible only. later gpu only and readback
  GpuCpuRange<std::byte> range{};
};

struct BufferDesc {
  size_t size{};
  MemoryType memory_type{MemoryType::GpuOnly};
};

struct Buffer {
  wi::allocator::shared_ptr<void> internal_data;
  BufferDesc desc;
  void* cpu_ptr{};
  void* gpu_ptr{};
};

inline GpuCpuRange<std::byte> gpu_cpu_range(rhi::Buffer& buffer) {
  return {.cpu = reinterpret_cast<std::byte*>(buffer.cpu_ptr),
          .gpu = reinterpret_cast<std::byte*>(buffer.gpu_ptr),
          .size = buffer.desc.size};
}

struct RenderPassInfo {
  static constexpr uint32_t k_max_rt = 8;

  Format rt_formats[k_max_rt] = {};
  uint32_t rt_count = 0;
  Format ds_format = Format::Unknown;
  uint32_t sample_count = 1;

  constexpr uint64_t get_hash() const {
    union Hasher {
      struct {
        uint64_t rt_format_0 : 6;
        uint64_t rt_format_1 : 6;
        uint64_t rt_format_2 : 6;
        uint64_t rt_format_3 : 6;
        uint64_t rt_format_4 : 6;
        uint64_t rt_format_5 : 6;
        uint64_t rt_format_6 : 6;
        uint64_t rt_format_7 : 6;
        uint64_t ds_format : 6;
        uint64_t sample_count : 3;
      } bits;
      uint64_t value;
    } hasher = {};
    static_assert(sizeof(Hasher) == sizeof(uint64_t));
    hasher.bits.rt_format_0 = static_cast<uint64_t>(rt_formats[0]);
    hasher.bits.rt_format_1 = static_cast<uint64_t>(rt_formats[1]);
    hasher.bits.rt_format_2 = static_cast<uint64_t>(rt_formats[2]);
    hasher.bits.rt_format_3 = static_cast<uint64_t>(rt_formats[3]);
    hasher.bits.rt_format_4 = static_cast<uint64_t>(rt_formats[4]);
    hasher.bits.rt_format_5 = static_cast<uint64_t>(rt_formats[5]);
    hasher.bits.rt_format_6 = static_cast<uint64_t>(rt_formats[6]);
    hasher.bits.rt_format_7 = static_cast<uint64_t>(rt_formats[7]);
    hasher.bits.ds_format = static_cast<uint64_t>(ds_format);
    hasher.bits.sample_count = static_cast<uint64_t>(sample_count);
    return hasher.value;
  }

  static constexpr RenderPassInfo from(const SwapchainDesc& swapchain_desc) {
    RenderPassInfo info;
    info.rt_count = 1;
    info.rt_formats[0] = swapchain_desc.format;
    return info;
  }
};

struct PipelineHash {
  const Pipeline* pipeline = nullptr;
  uint64_t renderpass_hash = 0;

  constexpr bool operator==(const PipelineHash& other) const {
    return pipeline == other.pipeline && renderpass_hash == other.renderpass_hash;
  }

  constexpr uint64_t get_hash() const {
    union {
      const Pipeline* ptr;
      uint64_t value;
    } pso_hasher = {};
    static_assert(sizeof(pso_hasher) == sizeof(uint64_t));
    pso_hasher.ptr = pipeline;
    return (pso_hasher.value ^ (renderpass_hash << 1)) >> 1;
  }
};

enum class LoadOp : uint8_t { Load, Clear, DontCare };
enum class StoreOp : uint8_t { Store, DontCare };

union ClearValue {
  glm::vec4 color;
  struct {
    float depth;
    uint32_t stencil;
  } depth_stencil;
};

struct RenderAttInfo {
  enum class Type : uint8_t { Color, DepthStencil };

  Texture* image;
  int subresource{-1};
  Type type{Type::Color};
  LoadOp load_op{LoadOp::Load};
  StoreOp store_op{StoreOp::Store};
  ClearValue clear_value{};

  static RenderAttInfo color_att(Texture* image, LoadOp load_op = LoadOp::Load,
                                 ClearValue clear_value = {}, StoreOp store_op = StoreOp::Store,
                                 int subresource = -1) {
    return {.image = image,
            .subresource = subresource,
            .type = Type::Color,
            .load_op = load_op,
            .store_op = store_op,
            .clear_value = clear_value};
  }

  static RenderAttInfo depth_stencil_att(Texture* image, LoadOp load_op = LoadOp::Load,
                                         ClearValue clear_value = {},
                                         StoreOp store_op = StoreOp::Store, int subresource = -1) {
    return {.image = image,
            .subresource = subresource,
            .type = Type::DepthStencil,
            .load_op = load_op,
            .store_op = store_op,
            .clear_value = clear_value};
  }
};

enum class StencilOp : uint8_t {
  Keep = 0,
  Zero,
  Replace,
  IncrementAndClamp,
  DecrementAndClamp,
  IncrementAndWrap,
  DecrementAndWrap,
};

enum class CompareOp : uint8_t {
  Never = 0,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  NotEqual,
  GreaterOrEqual,
  Always
};

enum class PrimitiveTopology : uint8_t {
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip,
  TriangleFan,
  PatchList
};

enum class FilterMode : uint8_t { Nearest, Linear };

enum class AddressMode : uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge
};

enum class BorderColor : uint8_t {
  FloatTransparentBlack,
  IntTransparentBlack,
  FloatOpaqueBlack,
  IntOpaqueBlack,
  FloatOpaqueWhite,
  IntOpaqueWhite
};

// allocate a heap. make it resident.

} // namespace gfx::rhi
