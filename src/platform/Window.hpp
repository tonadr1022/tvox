#pragma once

struct SDL_Window;

class Window {
public:
  virtual void init();

  void shutdown();
  SDL_Window *get_window() { return window_; }

protected:
  SDL_Window *window_{};
};

Window create_window();
