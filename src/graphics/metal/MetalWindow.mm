#include "MetalWindow.hpp"

#include <QuartzCore/CAMetalLayer.hpp>

#import <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>
#include <SDL3/SDL.h>

namespace gfx::metal {

void set_layer_for_window(SDL_Window *window, CA::MetalLayer *metal_layer) {
  SDL_PropertiesID props = SDL_GetWindowProperties(window);
  auto *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(
      props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  nswindow.contentView.layer = (__bridge CAMetalLayer *)metal_layer;
  nswindow.contentView.wantsLayer = true;
}

} // namespace gfx::metal
