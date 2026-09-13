#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace core {

/// Returns false on error
[[nodiscard]] bool read_bytes(const std::filesystem::path& path, std::vector<uint8_t>& out,
                              std::string& error);

/// Create parent directories if needed

[[nodiscard]] bool write_bytes(const std::filesystem::path& path, const void* data, size_t size,
                               std::string& error);

}  // namespace core
