#include "App.hpp"

#include <SDL3/SDL.h>

#include "core/Logger.hpp"
#include "platform/Window.hpp"

App::App() : window_(create_window()) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LINFO("failed to initialize SDL: {}", SDL_GetError());
    std::exit(1);
  }
  SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");

  window_.init();
  SDL_PumpEvents();

  renderer_.init(window_.get_window());
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
