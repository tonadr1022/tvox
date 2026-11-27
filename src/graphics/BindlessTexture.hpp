#pragma once

#include "graphics/rhi/Graphics.hpp"
namespace gfx {

struct BindlessTexture : rhi::Texture {
  uint32_t index;
};

} // namespace gfx
