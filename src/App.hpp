#pragma once

#include <memory>

#include "graphics/rhi/Device.hpp"
#include "platform/Window.hpp"

class App {
 public:
  App();
  void run();
  void shutdown();

 private:
  Window window_;
  std::unique_ptr<gfx::rhi::Device> device_;
};
