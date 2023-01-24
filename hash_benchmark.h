#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "benchmark.h"

// Returns a set of distinct number of cardinality `size`.  Returns the same set
// if we run this several times within a single process (but the set may be
// different from run-to-run).
const std::vector<uint64_t> GetSomeNumbers(size_t size);
// Return a set of distinct numbers of cardinality `size`, but it doesn't
// interesect `GetSomeNumbers(size)`.  Return the same set within several runs
// in the same process.
const std::vector<uint64_t> GetSomeOtherNumbers(size_t size);

template<class HashSet>
void IntHashSetBenchmark(size_t size) {
  const auto& values = GetSomeNumbers(size);
  Benchmark(
      [&]() {
        HashSet set;
        for (uint64_t value : values) {
          set.insert(value);
        }
        return values.size();
      },
      "insert   ", "insertion");
  HashSet set;
  for (uint64_t value : values) {
    set.insert(value);
  }
  Benchmark(
      [&]() {
        for (uint64_t value : values) {
          bool contains = set.contains(value);
          assert(contains);
          DoNotOptimize(contains);
        }
        return values.size();
      },
      "contains ", "found    ");
  const auto& not_values = GetSomeOtherNumbers(size);
  Benchmark(
      [&]() {
        for (uint64_t value : not_values) {
          bool contains = set.contains(value);
          assert(!contains);
          DoNotOptimize(contains);
        }
        return not_values.size();
      },
      "!contains", "not_found");
  Benchmark([&]() {
      for (uint64_t value : values) {
        DoNotOptimize(value);
      }
      return values.size();
    }, "nothing  ", "nop      ");
}

#endif  // HASH_BENCHMARK_H_
