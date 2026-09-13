#pragma once

#include <glm/vec4.hpp>

#include "graphics/rhi/ShaderType.hpp"
#include "small_vector/small_vector.hpp"
#include "wicked_engine/wiAllocator.h"

struct SDL_Window;

namespace gfx::rhi {

struct SwapchainDesc {
  uint32_t width, height;
};

struct Swapchain {
  wi::allocator::shared_ptr<void> internal_data;
  SwapchainDesc desc;

  [[nodiscard]] const SwapchainDesc& get_desc() const { return desc; }
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

struct ShaderCreateInfo {
  std::string path;
  ShaderType type;
  std::vector<std::string> defines;
  std::string entry_point{"main"};
};

struct GraphicsPipelineCreateInfo {
  struct Rasterization {
    bool depth_clamp{false};
    bool depth_bias{false};
    bool rasterize_discard_enable{false};
    // PolygonMode polygon_mode{PolygonMode::Fill};
    float line_width{1.};
    float depth_bias_constant_factor{};
    float depth_bias_clamp{};
    float depth_bias_slope_factor{};
  };

  // struct ColorBlendAttachment {
  //   bool enable{false};
  //   BlendFactor src_color_factor;
  //   BlendFactor dst_color_factor;
  //   BlendOp color_blend_op;
  //   BlendFactor src_alpha_factor;
  //   BlendFactor dst_alpha_factor;
  //   BlendOp alpha_blend_op;
  //   ColorComponentFlags color_write_mask{ColorComponentRBit | ColorComponentGBit |
  //                                        ColorComponentBBit | ColorComponentABit};
  // };
  // struct Blend {
  //   bool logic_op_enable{false};
  //   LogicOp logic_op{LogicOp::Copy};
  //   gch::small_vector<ColorBlendAttachment, k_max_color_attachments> attachments;
  //   float blend_constants[k_max_color_attachments]{};
  // };

  // struct Multisample {
  //   // TODO: flesh out, for now not caring about it
  //   SampleCountFlagBits rasterization_samples{SampleCount1Bit};
  //   float min_sample_shading{0.};
  //   bool sample_shading_enable{false};
  //   bool alpha_to_coverage_enable{false};
  //   bool alpha_to_one_enable{false};
  // };

  // struct StencilOpState {
  //   StencilOp fail_op;
  //   StencilOp pass_op;
  //   StencilOp depth_fail_op;
  //   CompareOp compare_op;
  //   uint32_t compare_mask;
  //   uint32_t write_mask;
  //   uint32_t reference;
  // };
  //
  // struct DepthStencil {
  //   StencilOpState stencil_front{};
  //   StencilOpState stencil_back{};
  //   float min_depth_bounds{0.};
  //   float max_depth_bounds{1.};
  //   bool depth_test_enable{false};
  //   bool depth_write_enable{false};
  //   CompareOp depth_compare_op{CompareOp::Never};
  //   bool depth_bounds_test_enable{false};
  //   bool stencil_test_enable{false};
  // };

  // TODO: use pointers?
  gch::small_vector<ShaderCreateInfo*, 3> shaders;

  PrimitiveTopology topology{PrimitiveTopology::TriangleList};
  // RenderTargetInfo rendering;
  Rasterization rasterization;
  // Blend blend;
  // Multisample multisample;
  // DepthStencil depth_stencil;

  // static constexpr DepthStencil depth_disable() { return DepthStencil{.depth_test_enable =
  // false}; } static constexpr DepthStencil depth_enable(bool write_enable, CompareOp op) {
  //   return DepthStencil{
  //       .depth_test_enable = true, .depth_write_enable = write_enable, .depth_compare_op = op};
  // }
  std::string name;
};

}  // namespace gfx::rhi
