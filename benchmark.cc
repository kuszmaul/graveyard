#include "benchmark.h"

#include <time.h>

#include <cstdint>
#include <functional>

namespace {
inline uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}
}  // namespace

void Benchmark(std::ofstream& output, std::function<void(size_t count)> setup,
               std::function<size_t()> fun, const std::vector<size_t>& counts) {
  for (size_t count : counts) {
    setup(count);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t memory_size = fun();
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    output << count << "," << end - start << "," << memory_size << std::endl;
  }
}
