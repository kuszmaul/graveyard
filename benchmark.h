#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <time.h>

#include <iostream>

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

template <class Fun>
double Benchmark(Fun fun, std::string_view description,
                 std::string_view item_name) {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t count = fun();
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  double ns_per_op = (end - start) / double(count);
  std::cerr << description << ": " << ns_per_op << "ns/" << item_name
            << std::endl;
  return ns_per_op;
}

#endif  // BENCHMARK_H_
