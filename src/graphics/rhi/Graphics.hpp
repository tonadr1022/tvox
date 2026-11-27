#pragma once

#include "core/Assert.hpp"
#include "wicked_engine/wiAllocator.h"

#include <cstdint>
#include <glm/vec4.hpp>
#include <span>

struct SDL_Window;

namespace gfx::rhi {

enum class Stage : uint64_t {
  None = 0,
  Indirect = 1ull << 0u,
  IndexInput = 1ull << 1u,
  Vertex = 1ull << 2u,
  Task = 1ull << 3u,
  Mesh = 1ull << 4u,
  DepthStencilTests = 1ull << 5u,
  Fragment = 1ull << 6u,
  ColorOutput = 1ull << 7u,
  Compute = 1ull << 8u,
  Transfer = 1ull << 9u,
  Host = 1ull << 10u,        // Barrier destination only, paired with HostRead.
  AllCommands = 1ull << 11u, // All GPU command stages; excludes host.
};

constexpr Stage operator|(Stage lhs, Stage rhs) noexcept {
  return static_cast<Stage>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
}

struct StageMask {
  Stage value;

  constexpr explicit operator bool() const noexcept {
    return static_cast<uint64_t>(value) != 0;
  }
};

constexpr StageMask operator&(Stage lhs, Stage rhs) noexcept {
  return {static_cast<Stage>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs))};
}

enum class Access : uint64_t {
  None = 0,
  TransferRead = 1ull << 0u,
  TransferWrite = 1ull << 1u,
  ShaderRead = 1ull << 2u,
  ShaderWrite = 1ull << 3u,
  ColorRead = 1ull << 4u,
  ColorWrite = 1ull << 5u,
  DepthStencilRead = 1ull << 6u,
  DepthStencilWrite = 1ull << 7u,
  IndirectRead = 1ull << 8u,
  IndexRead = 1ull << 9u,
  HostRead = 1ull << 10u,
  DescriptorRead = 1ull << 11u,
};

constexpr Access operator|(Access lhs, Access rhs) noexcept {
  return static_cast<Access>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
}

struct AccessMask {
  Access value;

  constexpr explicit operator bool() const noexcept {
    return static_cast<uint64_t>(value) != 0;
  }
};

constexpr AccessMask operator&(Access lhs, Access rhs) noexcept {
  return {static_cast<Access>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs))};
}

constexpr size_t k_max_frames_in_flight = 2;

enum class Format : uint8_t {
  Unknown = 0,
  Bgra8Unorm,
  Rgba8Unorm,
  Rgba32Float,
  D32Float,
};

enum class TextureType : uint8_t {
  D1,
  D2,
  D3,
};

struct SwapchainDesc {
  uint32_t width, height;
  Format format{Format::Bgra8Unorm};
};

struct Swapchain {
  wi::allocator::shared_ptr<void> internal_data;
  SwapchainDesc desc;
};

enum class TextureUsage : uint32_t {
  None = 0,
  RenderTarget = 1 << 0,
  ShaderRead = 1 << 1,
  ShaderWrite = 1 << 2,
};

struct TextureUsageMask {
  TextureUsage value;

  constexpr explicit operator bool() const noexcept {
    return static_cast<std::underlying_type_t<TextureUsage>>(value) != 0;
  }
};

constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept {
  using Underlying = std::underlying_type_t<TextureUsage>;
  return static_cast<TextureUsage>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

constexpr TextureUsage& operator|=(TextureUsage& lhs, TextureUsage rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

constexpr TextureUsageMask operator&(TextureUsage lhs, TextureUsage rhs) noexcept {
  using Underlying = std::underlying_type_t<TextureUsage>;
  return TextureUsageMask{
      static_cast<TextureUsage>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)),
  };
}

struct TextureDesc {
  uint32_t width{}, height{}, depth{};
  uint32_t array_length{1};
  uint32_t mip_count{1};
  Format format{};
  TextureUsage usage{TextureUsage::None};
  TextureType type{TextureType::D2};
};

struct Texture {
  wi::allocator::shared_ptr<void> internal_data;
  TextureDesc desc{};
};

struct RenderViewDesc {
  rhi::Texture& texture;
  uint32_t mip{};
  uint32_t slice{};
};

struct TextureDescriptorDesc {
  uint32_t slice{};
  uint32_t array_length{}; // 0 => remaining layers
  uint32_t mip{};
  uint32_t mip_count{}; // 0 => remaining mips
  TextureType texture_type{TextureType::D2};
};

struct RenderView {
  rhi::Texture* tex{};
  uint32_t slice{};
  uint32_t mip{};
};

struct ShaderDesc {
  std::string path;
};

struct Shader {
  wi::allocator::shared_ptr<void> internal_data;
  ShaderDesc desc{};
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

struct CmdPool {
  wi::allocator::shared_ptr<void> internal_data;
};

struct TimelineSemaphore {
  wi::allocator::shared_ptr<void> internal_data;
};

struct TimelinePoint {
  TimelineSemaphore sema;
  uint64_t value{};
};

inline GpuCpuRange<std::byte> gpu_cpu_range(rhi::Buffer& buffer) {
  return {
      .cpu = reinterpret_cast<std::byte*>(buffer.cpu_ptr),
      .gpu = reinterpret_cast<std::byte*>(buffer.gpu_ptr),
      .size = buffer.desc.size,
  };
}

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

  RenderView render_view{};
  Type type{Type::Color};
  LoadOp load_op{LoadOp::Load};
  StoreOp store_op{StoreOp::Store};
  ClearValue clear{};

  static RenderAttInfo color_att(RenderView render_view, LoadOp load_op = LoadOp::Load,
                                 ClearValue clear_value = {}, StoreOp store_op = StoreOp::Store) {
    return {
        .render_view = render_view,
        .type = Type::Color,
        .load_op = load_op,
        .store_op = store_op,
        .clear = clear_value,
    };
  }

  static RenderAttInfo depth_stencil_att(RenderView render_view, LoadOp load_op = LoadOp::Load,
                                         ClearValue clear_value = {},
                                         StoreOp store_op = StoreOp::Store) {
    return {
        .render_view = render_view,
        .type = Type::DepthStencil,
        .load_op = load_op,
        .store_op = store_op,
        .clear = clear_value,
    };
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
  Always,
};

enum class PrimitiveTopology : uint8_t {
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip,
  TriangleFan,
  PatchList,
};

enum class FilterMode : uint8_t { Nearest, Linear };

enum class AddressMode : uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge,
};

enum class BorderColor : uint8_t {
  FloatTransparentBlack,
  IntTransparentBlack,
  FloatOpaqueBlack,
  IntOpaqueBlack,
  FloatOpaqueWhite,
  IntOpaqueWhite,
};

// TODO: make this backend specific. rhi doesn't need to know about this
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

  static constexpr RenderPassInfo from(std::span<RenderAttInfo> color_atts,
                                       RenderAttInfo* depth_att) {
    RenderPassInfo info;
    info.rt_count = color_atts.size();
    for (size_t i = 0; i < color_atts.size(); i++) {
      auto& color_att = color_atts[i];
      // TODO: get the format of the real texture
      info.rt_formats[i] = color_att.render_view.tex->desc.format;
      ASSERT(info.rt_formats[i] != Format::Unknown);
    }
    if (depth_att) {
      info.ds_format = depth_att->render_view.tex->desc.format;
    }
    info.sample_count = 1;
    return info;
  }
};

// allocate a heap. make it resident.

} // namespace gfx::rhi
