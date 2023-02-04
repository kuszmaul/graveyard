#include "benchmark.h"

#include <time.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>

namespace {
inline uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}
}  // namespace

void Benchmark(std::ofstream& output, std::function<void(size_t count)> setup,
               std::function<size_t()> fun, const std::vector<size_t>& counts) {
  constexpr size_t kNumberOfTrials = 5;
  constexpr double kNInverse = 1.0 / kNumberOfTrials;
  for (size_t count : counts) {
    setup(count);
    uint64_t x_sum = 0;
    uint64_t x_squared_sum = 0;
    size_t memory_size;
    (std::cerr << " " << count).flush();
    for (size_t trial = 0; trial < kNumberOfTrials; ++trial) {
      struct timespec start;
      clock_gettime(CLOCK_MONOTONIC, &start);
      memory_size = fun();
      struct timespec end;
      clock_gettime(CLOCK_MONOTONIC, &end);
      uint64_t elapsed = end - start;
      x_sum += elapsed;
      x_squared_sum += elapsed * elapsed;
      output << "#" << count << "," << elapsed << "," << memory_size << std::endl;
      (std::cerr << ".").flush();
    }
    // These variables are named as if in Reverse Polish Notation.
    double x_expected = x_sum * kNInverse;
    double x_expected_square = x_expected * x_expected;
    double x_square_expected = x_squared_sum * kNInverse;
    double standard_deviation = sqrt(x_square_expected - x_expected_square);
    output << count << "," << x_expected << "," << memory_size << "," << 2 * standard_deviation << std::endl;
  }
}
