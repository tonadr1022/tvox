#pragma once

#include <filesystem>

namespace core {

/// Directory containing the running executable. Empty on failure.
[[nodiscard]] std::filesystem::path executable_directory();

}  // namespace core
