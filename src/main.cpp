#include "App.hpp"

#include <charconv>
#include <cstdint>
#include <expected>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
std::expected<std::optional<uint64_t>, std::string> parse_args(int argc, char** argv) {
  std::optional<uint64_t> frame_limit;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    constexpr std::string_view option{"--frames"};
    std::string_view value;

    if (arg == option) {
      if (++i == argc) {
        return std::unexpected{"--frames requires a positive integer"};
      }
      value = argv[i];
    } else if (arg.starts_with("--frames=")) {
      value = arg.substr(option.size() + 1);
    } else {
      return std::unexpected{"usage: tvox [--frames count]"};
    }

    uint64_t frames{};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), frames);
    if (error != std::errc{} || end != value.data() + value.size() || frames == 0) {
      return std::unexpected{"--frames requires a positive integer"};
    }
    if (frame_limit) {
      return std::unexpected{"--frames may only be specified once"};
    }
    frame_limit = frames;
  }
  return frame_limit;
}
} // namespace

int main(int argc, char** argv) {
  auto args = parse_args(argc, argv);
  if (!args) {
    std::cerr << args.error() << '\n';
    return 2;
  }

  App app;
  app.run(*args);
}
