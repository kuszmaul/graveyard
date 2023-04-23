#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <time.h>

#include <cstdint>
#include <fstream>
#include <functional>
#include <vector> // for vector

#include "absl/flags/declare.h"

ABSL_DECLARE_FLAG(size_t, number_of_trials);

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp &value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

void Benchmark(std::ofstream &output,
               std::function<void(size_t count, size_t trial)> setup,
               std::function<size_t()> fun, // returns memory size
               const std::vector<size_t> &counts);

static inline constexpr uint64_t operator-(timespec a, timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

static inline timespec GetTime() {
  timespec result;
  clock_gettime(CLOCK_MONOTONIC, &result);
  return result;
}

#endif // BENCHMARK_H_
