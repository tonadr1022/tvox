#include "core/DynamicLib.hpp"

#include <dlfcn.h>

#include <sstream>

namespace core {

std::string shared_library_filename(std::string_view stem) {
#if defined(_WIN32)
  return std::string(stem) + ".dll";
#elif defined(__APPLE__)
  return std::string("lib") + std::string(stem) + ".dylib";
#else
  return std::string("lib") + std::string(stem) + ".so";
#endif
}

DynamicLib::~DynamicLib() { close(); }

DynamicLib::DynamicLib(DynamicLib&& other) noexcept : handle_(other.handle_) {
  other.handle_ = nullptr;
}

DynamicLib& DynamicLib::operator=(DynamicLib&& other) noexcept {
  if (this != &other) {
    close();
    handle_ = other.handle_;
    other.handle_ = nullptr;
  }
  return *this;
}

void DynamicLib::close() {
  if (handle_) {
    dlclose(handle_);
    handle_ = nullptr;
  }
}

bool DynamicLib::open(const std::filesystem::path& path, std::string& error) {
  close();
  handle_ = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (!handle_) {
    error = std::string("dlopen failed: ") + path.string() + " (" + dlerror() + ")";
    return false;
  }
  error.clear();
  return true;
}

bool DynamicLib::open_search(const std::string& filename,
                             const std::vector<std::filesystem::path>& dirs, std::string& error) {
  close();

  std::ostringstream tried;
  for (const auto& dir : dirs) {
    const auto candidate = dir / filename;
    if (!std::filesystem::exists(candidate)) {
      tried << "  missing: " << candidate << '\n';
      continue;
    }
    handle_ = dlopen(candidate.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (handle_) {
      error.clear();
      return true;
    }
    tried << "  dlopen failed: " << candidate << " (" << dlerror() << ")\n";
  }

  handle_ = dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (handle_) {
    error.clear();
    return true;
  }
  tried << "  dlopen failed: " << filename << " (" << dlerror() << ")\n";

  error = std::string("Failed to load ") + filename + ":\n" + tried.str();
  return false;
}

void* DynamicLib::symbol_raw(const char* name, std::string& error) const {
  if (!handle_) {
    error = std::string("dlsym(") + name + "): library not open";
    return nullptr;
  }
  dlerror();
  void* sym = dlsym(handle_, name);
  if (const char* err = dlerror()) {
    error = std::string("dlsym(") + name + "): " + err;
    return nullptr;
  }
  return sym;
}

}  // namespace core
