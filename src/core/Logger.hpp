#pragma once

#include <print>  // IWYU pragma: keep

#include "core/EAssert.hpp"

#define LINFO(...) std::println("[info]" __VA_ARGS__)
#define LWARN(...) std::println("[warn]     " __VA_ARGS__)
#define LERROR(...) std::println("[error]    " __VA_ARGS__)
#define LCRITICAL(...) std::println("[critical] " __VA_ARGS__)
#define LDEBUG(...) std::println("[debug]   " __VA_ARGS__)
#define LOG_ASSERT(cond, str, ...)   \
  {                                  \
    if (!(cond)) {                   \
      LCRITICAL(str, ##__VA_ARGS__); \
      ASSERT(cond);                  \
    }                                \
  }

#define FATAL_IF(expr, ...)   \
  do {                        \
    if (!(expr)) {            \
      LCRITICAL(__VA_ARGS__); \
      std::abort();           \
    }                         \
  } while (0)