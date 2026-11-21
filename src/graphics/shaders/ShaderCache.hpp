#pragma once

#include <expected>
#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "graphics/rhi/ShaderType.hpp"

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

  /// Ensure every stage of one technique is cooked.
  [[nodiscard]] EnsureStats ensure_technique(std::string_view name, bool force = false);

  [[nodiscard]] std::optional<std::string> is_outdated(std::string_view technique,
                                                       rhi::ShaderType stage) const;

  /// True if any unit previously returned by `load_shader` is outdated
  /// (watcher poll hook; no file watcher in v1).
  [[nodiscard]] bool any_registered_outdated() const;

  [[nodiscard]] size_t registered_shader_count() const;

  /// Load a cooked shader by technique name + stage.
  /// Successful loads are tracked for future hot-reload polling.
  [[nodiscard]] std::expected<std::vector<std::byte>, std::string> load_shader(
      std::string_view technique, rhi::ShaderType stage);

 private:
  CacheRoots roots_;
  mutable std::mutex registered_mu_;
  std::set<std::pair<std::string, rhi::ShaderType>> registered_shaders_;

  void register_loaded(std::string_view technique, rhi::ShaderType stage);
};

}  // namespace gfx
