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

std::filesystem::path find_upwards(const std::filesystem::path& start,
                                   const std::filesystem::path& relative_marker) {
  std::error_code ec;
  std::filesystem::path dir = std::filesystem::absolute(start, ec);
  if (ec) {
    dir = start;
  }

  while (true) {
    if (std::filesystem::is_directory(dir / relative_marker, ec)) {
      return dir;
    }
    const std::filesystem::path parent = dir.parent_path();
    if (parent.empty() || parent == dir) {
      break;
    }
    dir = parent;
  }
  return {};
}

std::filesystem::path resolve_project_root() {
  if (auto root = find_upwards(std::filesystem::current_path(), "resources/shaders");
      !root.empty()) {
    return root;
  }
  if (auto exe = executable_directory(); !exe.empty()) {
    if (auto root = find_upwards(exe, "resources/shaders"); !root.empty()) {
      return root;
    }
  }
  return {};
}

}  // namespace core
