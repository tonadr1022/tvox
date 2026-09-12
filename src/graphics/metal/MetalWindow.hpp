#pragma once

struct SDL_Window;
namespace CA {
class MetalLayer;
}

namespace gfx::metal {

void set_layer_for_window(SDL_Window *window, CA::MetalLayer *metal_layer);

}
