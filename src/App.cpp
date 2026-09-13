#include "App.hpp"

#include <SDL3/SDL.h>

#include "core/Logger.hpp"
#include "core/Paths.hpp"
#include "graphics/Renderer.hpp"
#include "platform/Window.hpp"

App::App() : window_(create_window()) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LINFO("failed to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }
  SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");

  window_.init();
  SDL_PumpEvents();

  const std::filesystem::path project_root = core::resolve_project_root();
  FATAL_IF(project_root.empty(),
           "could not locate project root (resources/shaders); shaders will not cook");
  LINFO("project root: {}", project_root.string());

  auto resources_root = project_root / "resources";

  renderer_.init(gfx::Renderer::InitInfo{
      .window = window_.get_window(),
      .shader_root = resources_root / "shaders",
      .cache_root = resources_root / "shader_cache",
  });
}

void App::run() {
  bool close_requested{};
  while (!close_requested) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_QUIT: {
          close_requested = true;
          continue;
        }
        default:
          continue;
      }
    }

    renderer_.render();
  }

  shutdown();
}

void App::shutdown() {
  window_.shutdown();
  SDL_Quit();
}
