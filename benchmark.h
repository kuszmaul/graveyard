#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <time.h>

#include <algorithm>  // for max, min
#include <cmath>
#include <cstdint>  // for uint64_t
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>  // for operator<<, string_view

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

inline uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

class BenchmarkResult {
 public:
  void AddSample(double x_i) {
    ++n_;
    min_ns_ = min_ns_ ? std::min(x_i, *min_ns_) : x_i;
    max_ns_ = max_ns_ ? std::max(x_i, *max_ns_) : x_i;
    sum_x_ += x_i;
    sum_x2_ += x_i * x_i;
  }
  double Mean() const { return sum_x_ / n_; }
  double StandardDeviation() const {
    double mean = Mean();
    return sqrt(sum_x2_ / n_ - mean * mean);
  }
  void SetMemoryEstimate(size_t memory_size, size_t minimal_memory_estimate) { memory_size_ = memory_size; minimal_memory_estimate_ = minimal_memory_estimate; }
  size_t MemorySize() const { return memory_size_; }
  size_t MinimalMemoryEstimate() const { return minimal_memory_estimate_; }

 private:
  size_t n_ = 0;  // number of runs
  std::optional<double> min_ns_ = std::nullopt;
  std::optional<double> max_ns_ = std::nullopt;
  double sum_x_ = 0;   // sum of `x_i`.
  double sum_x2_ = 0;  // sum of `x_i_2`.
  size_t memory_size_ = 0; // an estimate of how much memory was actually used
  size_t minimal_memory_estimate_ = 0; // and estimate of how much memory is required (e.g., at 100% load factor).
};

// `fun` returns an estimate of the memory consumed
BenchmarkResult Benchmark(std::function<size_t()> fun, size_t count, size_t minimal_memory_estimate,
                          size_t n_runs);

#endif  // BENCHMARK_H_
