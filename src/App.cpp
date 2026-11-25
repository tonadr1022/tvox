#include "App.hpp"

#include "core/Logger.hpp"
#include "core/Paths.hpp"
#include "graphics/Renderer.hpp"
#include "platform/Window.hpp"

#include <SDL3/SDL.h>

App::App() : window_(create_window()) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LINFO("failed to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }

  window_.init();

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

void App::run(std::optional<uint64_t> max_frames) {
  bool shutdown_requested{};
  uint64_t rendered_frames{};
  while (!shutdown_requested && (!max_frames || rendered_frames < *max_frames)) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT: {
        shutdown_requested = true;
        continue;
      }
      default:
        continue;
      }
    }

    renderer_.render();
    ++rendered_frames;
  }

  shutdown();
}

void App::shutdown() {
  window_.shutdown();
  SDL_Quit();
}
