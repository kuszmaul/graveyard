#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <time.h>

#include <algorithm>  // for max, min
#include <cmath>
#include <cstdint>  // for uint64_t
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

 private:
  size_t n_ = 0;  // number of runs
  std::optional<double> min_ns_ = std::nullopt;
  std::optional<double> max_ns_ = std::nullopt;
  double sum_x_ = 0;   // sum of `x_i`.
  double sum_x2_ = 0;  // sum of `x_i_2`.
};

template <class Fun>
BenchmarkResult Benchmark(Fun fun, size_t n_runs = 20) {
  BenchmarkResult result;
  fun();  // Run the benchmark once, since the first run is often different.
  for (size_t run = 0; run < n_runs; ++run) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t count = fun();
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.AddSample((end - start) / double(count));
  }
  return result;
}

template <class Fun>
double Benchmark(Fun fun, std::string_view description,
                 std::string_view item_name) {
  BenchmarkResult result;
  uint64_t firstcount;
  {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    firstcount =
        fun();  // call fun once since the first run is often different.
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    // double ns_per_op = (end - start) / double(firstcount);
    // std::cerr << " first " << ns_per_op << std::endl;
  }
  for (size_t samples = 0; samples < 20; ++samples) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t count = fun();
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.AddSample((end - start) / double(count));
  }
  std::cerr << description << ": " << result.Mean() << "±"
            << result.StandardDeviation() * 2 << "ns/" << item_name
            << " count=" << firstcount << std::endl;
  return result.Mean();
}

#endif  // BENCHMARK_H_
