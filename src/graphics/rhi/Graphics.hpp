#pragma once

#include <glm/vec4.hpp>

#include "wicked_engine/wiAllocator.h"

struct SDL_Window;

namespace gfx::rhi {

struct SwapchainDesc {
  SDL_Window* window{};
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

}  // namespace gfx::rhi
