#pragma once

#include <cstddef>

#include "graphics/rhi/CmdEncoder.hpp"

namespace gfx::metal {

class MetalCmdEncoder : public rhi::CmdEncoder {
 public:
  size_t get_encoder_index() { return 0; }

 private:
};

}  // namespace gfx::metal
