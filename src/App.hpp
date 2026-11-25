#pragma once

#include "graphics/Renderer.hpp"
#include "platform/Window.hpp"

#include <cstdint>
#include <optional>

class App {
public:
  App();
  void run(std::optional<uint64_t> max_frames = std::nullopt);
  void shutdown();

private:
  Window window_;
  gfx::Renderer renderer_;
};
