#pragma once

#include <filesystem>

namespace core {

/// Directory containing the running executable. Empty on failure.
[[nodiscard]] std::filesystem::path executable_directory();

/// Walk upwards from `start` until `relative_marker` exists as a directory.
/// Returns the directory that contains the marker, or empty on failure.
[[nodiscard]] std::filesystem::path find_upwards(const std::filesystem::path& start,
                                                 const std::filesystem::path& relative_marker);

/// Project root: directory containing `resources/shaders`. Searches cwd then executable dir.
[[nodiscard]] std::filesystem::path resolve_project_root();

}  // namespace core
