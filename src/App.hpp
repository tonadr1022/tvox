#pragma once

#include "graphics/Renderer.hpp"
#include "platform/Window.hpp"

class App {
 public:
  App();
  void run();
  void shutdown();

 private:
  Window window_;
  gfx::Renderer renderer_;
};
