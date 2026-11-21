#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace core {

[[nodiscard]] std::expected<std::vector<std::byte>, std::string> read_binary_file(
    const std::filesystem::path& path);

struct WriteOptions {
  bool make_parent_directories{true};
};

[[nodiscard]] std::expected<void, std::string> write_binary_file(
    const std::filesystem::path& path, std::span<const std::byte>,
    const WriteOptions& write_opts = {});

[[nodiscard]] std::expected<void, std::string> write_text_file(const std::filesystem::path& path,
                                                               std::string_view text,
                                                               const WriteOptions& write_opts = {});

}  // namespace core
