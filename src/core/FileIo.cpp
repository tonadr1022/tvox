#include "core/FileIo.hpp"

#include <fstream>

namespace core {

bool read_bytes(const std::filesystem::path& path, std::vector<uint8_t>& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "Failed to read: " + path.string();
    return false;
  }
  file.seekg(0, std::ios::end);
  const std::streamoff size = file.tellg();
  if (size < 0) {
    error = "Failed to size: " + path.string();
    return false;
  }
  file.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(size));
  if (size > 0 && !file.read(reinterpret_cast<char*>(out.data()), size)) {
    error = "Failed to read bytes: " + path.string();
    return false;
  }
  return true;
}

bool write_bytes(const std::filesystem::path& path, const void* data, size_t size,
                 std::string& error) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    error = "Failed to create directory for: " + path.string();
    return false;
  }
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    error = "Failed to open for write: " + path.string();
    return false;
  }
  if (size > 0 && !file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size))) {
    error = "Failed to write: " + path.string();
    return false;
  }
  return true;
}

}  // namespace core
