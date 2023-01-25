#include "benchmark.h"

#include <functional>

BenchmarkResult Benchmark(std::function<size_t()> fun, size_t count,
                          size_t n_runs) {
  BenchmarkResult result;
  fun();  // Run the benchmark once, since the first run is often different.
  size_t memory_estimate = 0;
  for (size_t run = 0; run < n_runs; ++run) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t memory = fun();
    memory_estimate = memory;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.AddSample((end - start) / double(count));
  }
  result.SetMemoryEstimate(memory_estimate);
  return result;
}
