#include "Window.hpp"

#include "apple/AppleWindow.hpp"
#include "core/Logger.hpp"

#include <SDL3/SDL.h>

void Window::init() {
  SDL_WindowFlags window_flags{SDL_WINDOW_HIGH_PIXEL_DENSITY};

#ifdef __APPLE__
  window_flags |= SDL_WINDOW_METAL;
#endif

  window_ = SDL_CreateWindow("TVOX", 800, 800, window_flags);
}

void Window::shutdown() { SDL_DestroyWindow(window_); }

Window create_window() {
#ifdef __APPLE__
  return AppleWindow{};
#endif
  LCRITICAL("Unsupported OS");
}
