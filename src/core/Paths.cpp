#include "core/Paths.hpp"

#include <SDL3/SDL.h>

namespace core {

std::filesystem::path executable_directory() {
  const char* base = SDL_GetBasePath();
  if (!base || !*base) {
    return {};
  }
  return {base};
}

}  // namespace core
