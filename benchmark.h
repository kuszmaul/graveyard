#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <time.h>

#include <algorithm>    // for max, min
#include <cmath>
#include <cstdint>      // for uint64_t
#include <iostream>
#include <limits>
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

template <class Fun>
double Benchmark(Fun fun, std::string_view description,
                 std::string_view item_name) {
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::min();
  double n = 0;
  double sum_x = 0;   // sum of x_i
  double sum_x2 = 0;  // sum of x_i^2
  uint64_t firstcount;
  {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    firstcount = fun(); // call fun once since the first run is often different.
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    //double ns_per_op = (end - start) / double(firstcount);
    //std::cerr << " first " << ns_per_op << std::endl;
  }
  for (size_t samples = 0; samples < 20; ++samples) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t count = fun();
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    double ns_per_op = (end - start) / double(count);
    //std::cerr << " " << ns_per_op << std::endl;
    min = std::min(ns_per_op, min);
    max = std::max(ns_per_op, max);
    sum_x += ns_per_op;
    sum_x2 += ns_per_op*ns_per_op;
    ++n;
  }
  double Ex = sum_x / n;
  double E_x2 = sum_x2 / n;
  double stddev = sqrt(E_x2 - Ex * Ex);
  std::cerr << description << ": " << Ex << "±" << stddev * 2 << "ns/" << item_name
            << " count=" << firstcount << std::endl;
  return Ex;
}

#endif  // BENCHMARK_H_
