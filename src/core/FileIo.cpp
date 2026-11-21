#include "core/FileIo.hpp"

#include <expected>
#include <format>
#include <fstream>

namespace core {

std::expected<std::vector<std::byte>, std::string>
read_binary_file(const std::filesystem::path& path) {
  std::vector<std::byte> out;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected{std::format("Failed to read: {}", path.string())};
  }
  file.seekg(0, std::ios::end);
  const std::streamoff size = file.tellg();
  if (size < 0) {
    return std::unexpected{std::format("Failed to size: {}", path.string())};
  }
  file.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(size));
  if (size > 0 && !file.read(reinterpret_cast<char*>(out.data()), size)) {
    return std::unexpected{std::format("Failed to read bytes: {}", path.string())};
  }
  return out;
}

std::expected<void, std::string> write_binary_file(const std::filesystem::path& path,
                                                   std::span<const std::byte> data,
                                                   const WriteOptions& write_opts) {
  if (write_opts.make_parent_directories) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
      return std::unexpected{std::format("Failed to create directory for: {}", path.string())};
    }
  }
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    return std::unexpected{std::format("Failed to open for write: {}", path.string())};
  }
  if (data.size() > 0 && !file.write(reinterpret_cast<const char*>(data.data()),
                                     static_cast<std::streamsize>(data.size()))) {
    return std::unexpected{std::format("Failed to write: {}", path.string())};
  }

  return {};
}

std::expected<void, std::string> write_text_file(const std::filesystem::path& path,
                                                 std::string_view text,
                                                 const WriteOptions& write_opts) {
  return write_binary_file(
      path,
      std::span<const std::byte>(reinterpret_cast<const std::byte*>(text.data()), text.size()),
      write_opts);
}

} // namespace core
