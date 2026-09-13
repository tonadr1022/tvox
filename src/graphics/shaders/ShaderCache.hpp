#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/shaders/ShaderCompiler.hpp"

namespace gfx {

class ShaderCache {
 public:
  struct CacheRoots {
    std::filesystem::path shader_root;
    std::filesystem::path cache_root;
  };

  explicit ShaderCache(CacheRoots roots);

  struct EnsureStats {
    size_t compiled{0};
    size_t up_to_date{0};
    size_t failed{0};
    std::string first_error;
  };

  /// Ensure every stage in `TechniqueRegistry` is cooked.
  [[nodiscard]] EnsureStats ensure_all(bool force = false);

  /// Load a cooked metallib by technique name + stage.
  [[nodiscard]] bool load_metallib(std::string_view technique, rhi::ShaderType stage,
                                   std::vector<uint8_t>& out, std::string* error = nullptr) const;

 private:
  CacheRoots roots_;
};

}  // namespace gfx
