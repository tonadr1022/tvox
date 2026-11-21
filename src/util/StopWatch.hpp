#include <chrono>

class Stopwatch {
public:
  using Clock = std::chrono::steady_clock;

  Stopwatch() : start_time_(Clock::now()) {
  }

  void reset() {
    start_time_ = Clock::now();
  }

  double elapsed_seconds() const {
    return std::chrono::duration<double>(Clock::now() - start_time_).count();
  }

  size_t elapsed_milliseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time_)
        .count();
  }

  size_t elapsed_microseconds() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start_time_)
        .count();
  }

private:
  Clock::time_point start_time_;
};
