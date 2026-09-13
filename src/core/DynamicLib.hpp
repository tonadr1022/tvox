#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace core {

/// Platform shared-library filename for a stem.
/// e.g. "dxcompiler" -> "libdxcompiler.dylib" / "libdxcompiler.so" / "dxcompiler.dll"
[[nodiscard]] std::string shared_library_filename(std::string_view stem);

class DynamicLib {
public:
  DynamicLib() = default;
  ~DynamicLib();

  DynamicLib(const DynamicLib&) = delete;
  DynamicLib& operator=(const DynamicLib&) = delete;
  DynamicLib(DynamicLib&& other) noexcept;
  DynamicLib& operator=(DynamicLib&& other) noexcept;

  /// Open an absolute or relative path.
  [[nodiscard]] bool open(const std::filesystem::path& path, std::string& error);

  /// Try `dirs[i]/filename`, then bare `filename` via the default loader search.
  [[nodiscard]] bool open_search(const std::string& filename,
                                 const std::vector<std::filesystem::path>& dirs,
                                 std::string& error);

  template <typename Fn>
  [[nodiscard]] Fn symbol(const char* name, std::string& error) const {
    return reinterpret_cast<Fn>(symbol_raw(name, error));
  }

  [[nodiscard]] explicit operator bool() const { return handle_ != nullptr; }
  [[nodiscard]] void* native() const { return handle_; }

private:
  [[nodiscard]] void* symbol_raw(const char* name, std::string& error) const;
  void close();

  void* handle_{nullptr};
};

}  // namespace core
