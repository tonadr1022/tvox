#pragma once

#include "core/Logger.hpp"
#include <cstdlib>
#include <print>

#define ALL_ASSERTS_ENABLED 1

#ifndef NDEBUG
#ifndef ALL_ASSERTS_ENABLED
#define ALL_ASSERTS_ENABLED 1
#endif
#endif
// TODO: clean up this impl

class AlwaysAssert {
public:
  static void fail(const char* expr, const char* file, int line) {
    std::println(stderr, "Assertion failed: ({}), file {}, line {}", expr, file, line);
    std::abort();
  }

  static void fail(const char* expr, const char* file, int line, const char* msg) {
    std::println(stderr, "Assertion failed: ({}), file {}, line {}: {}", expr, file, line, msg);
    std::abort();
  }
};

#define ALWAYS_ASSERT(expr, ...)                                                                   \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      AlwaysAssert::fail(#expr, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__));                     \
    }                                                                                              \
  } while (0)

#ifdef ALL_ASSERTS_ENABLED
#define ASSERT(expr, ...)                                                                          \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      AlwaysAssert::fail(#expr, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__));                     \
    }                                                                                              \
  } while (0)
#else
#define ASSERT(expr, ...)
#endif

#define FATAL_IF(expr, ...)                                                                        \
  do {                                                                                             \
    if ((expr)) {                                                                                  \
      LCRITICAL(__VA_ARGS__);                                                                      \
      std::abort();                                                                                \
    }                                                                                              \
  } while (0)

#define FATAL(expr, ...)                                                                           \
  do {                                                                                             \
    LCRITICAL(__VA_ARGS__);                                                                        \
    std::abort();                                                                                  \
  } while (0)