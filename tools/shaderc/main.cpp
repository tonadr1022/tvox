#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "core/Paths.hpp"
#include "graphics/shaders/ShaderCache.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"

namespace fs = std::filesystem;

namespace {

constexpr const char* k_exe = "tvox-shaderc";

void usage(const char* argv0) {
  std::cerr << "usage: " << argv0
            << " [--project-root <dir>] [--force] (--all | <technique> [technique...])\n"
            << "\n"
            << "Cook HLSL techniques from TechniqueRegistry into resources/shader_cache/.\n"
            << "Uses the same in-process Compile() path as the app.\n";
}

void print_stats(std::string_view label, const gfx::ShaderCache::EnsureStats& stats) {
  std::cout << k_exe << ": " << label << ": " << stats.compiled << " compiled, " << stats.up_to_date
            << " up to date, " << stats.failed << " failed\n";
  if (!stats.first_error.empty()) {
    std::cerr << k_exe << ": " << stats.first_error << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> techniques;
  fs::path project_root;
  bool compile_all = false;
  bool force = false;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--project-root") {
      if (i + 1 >= argc) {
        std::cerr << k_exe << ": --project-root requires a directory\n";
        usage(argv[0]);
        return 2;
      }
      project_root = argv[++i];
    } else if (arg == "--all") {
      compile_all = true;
    } else if (arg == "--force") {
      force = true;
    } else if (arg == "-h" || arg == "--help") {
      usage(argv[0]);
      return 0;
    } else if (arg.starts_with('-')) {
      std::cerr << k_exe << ": unknown option: " << arg << '\n';
      usage(argv[0]);
      return 2;
    } else {
      techniques.emplace_back(arg);
    }
  }

  if (compile_all && !techniques.empty()) {
    std::cerr << k_exe << ": --all cannot be combined with technique names\n";
    usage(argv[0]);
    return 2;
  }
  if (!compile_all && techniques.empty()) {
    usage(argv[0]);
    return 2;
  }

  // SDL_GetBasePath (dylib / project discovery) wants an initialized subsystem.
  if (!SDL_Init(0)) {
    std::cerr << k_exe << ": SDL_Init failed: " << SDL_GetError() << '\n';
    return 1;
  }
  struct SdlQuit {
    ~SdlQuit() { SDL_Quit(); }
  } sdl_quit;

  if (project_root.empty()) {
    project_root = core::resolve_project_root();
  } else {
    project_root = fs::absolute(project_root);
  }
  if (project_root.empty() || !fs::is_directory(project_root / "resources" / "shaders")) {
    std::cerr << k_exe
              << ": could not locate project root (resources/shaders); pass --project-root\n";
    return 1;
  }

  const fs::path resources = project_root / "resources";
  gfx::ShaderCache cache({
      .shader_root = resources / "shaders",
      .cache_root = resources / "shader_cache",
  });

  gfx::ShaderCache::EnsureStats stats;
  if (compile_all) {
    stats = cache.ensure_all(force);
    print_stats("--all", stats);
  } else {
    for (const std::string& name : techniques) {
      if (!gfx::ShaderTechniqueRegistry::find(name)) {
        std::cerr << k_exe << ": unknown technique '" << name << "'\n";
        std::cerr << k_exe << ": known techniques:";
        for (const gfx::ShaderTechniqueDesc& tech : gfx::ShaderTechniqueRegistry::all()) {
          std::cerr << ' ' << tech.name;
        }
        std::cerr << '\n';
        return 1;
      }
      auto one = cache.ensure_technique(name, force);
      stats.compiled += one.compiled;
      stats.up_to_date += one.up_to_date;
      stats.failed += one.failed;
      if (stats.first_error.empty()) {
        stats.first_error = std::move(one.first_error);
      }
    }
    print_stats("selected", stats);
  }

  return stats.failed > 0 ? 1 : 0;
}
