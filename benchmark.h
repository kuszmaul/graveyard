#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <cstddef>
#include <fstream>
#include <functional>
#include <vector>  // for vector

#include "absl/flags/declare.h"

ABSL_DECLARE_FLAG(size_t, number_of_trials);

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

void Benchmark(std::ofstream& output,
               std::function<void(size_t count, size_t trial)> setup,
               std::function<size_t()> fun,  // returns memory size
               const std::vector<size_t>& counts);

#endif  // BENCHMARK_H_
