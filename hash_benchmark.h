#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <bits/utility.h>  // for tuple_element<>::type

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>  // for operator<<, basic_ostream, basic_ostream<>...
#include <map>
#include <string>       // for operator<, string, operator<<, char_traits
#include <string_view>  // for string_view
#include <type_traits>  // for add_const<>::type
#include <utility>      // for move
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

class HashBenchmarkResults {
 public:
  void Add(std::string_view implementation, std::string_view operation,
           size_t size, BenchmarkResult result);
  void Print() const;
  struct Key {
    std::string implementation;
    std::string operation;
    size_t input_size;

   private:
    friend bool operator<(const Key& a, const Key& b);
  };
 private:
  std::map<Key, BenchmarkResult> results_;
};

template <class HashSet>
void IntHashSetBenchmark(HashBenchmarkResults& results,
                         std::function<size_t(const HashSet&)> memory_estimator,
                         std::string_view implementation, size_t size,
                         size_t n_runs = 20) {
  const auto& values = GetSomeNumbers(size);
  size_t minimal_memory_consumption = size * sizeof(typename HashSet::value_type);
  results.Add(implementation, "insert", size,
              Benchmark(
                  [&]() {
                    HashSet set;
                    for (uint64_t value : values) {
                      set.insert(value);
                    }
                    return memory_estimator(set);
                  },
                  size, minimal_memory_consumption, n_runs));
  HashSet set;
  for (uint64_t value : values) {
    set.insert(value);
  }
  results.Add(implementation, "contains(true)", size,
              Benchmark(
                  [&]() {
                    for (uint64_t value : values) {
                      bool contains = set.contains(value);
                      assert(contains);
                      DoNotOptimize(contains);
                    }
                    return memory_estimator(set);
                  },
                  size, minimal_memory_consumption, n_runs));
  const auto& not_values = GetSomeOtherNumbers(size);
  results.Add(implementation, "contains(false)", size,
              Benchmark(
                  [&]() {
                    for (uint64_t value : not_values) {
                      bool contains = set.contains(value);
                      assert(!contains);
                      DoNotOptimize(contains);
                    }
                    return memory_estimator(set);
                  },
                  size, minimal_memory_consumption, n_runs));
  results.Add("nop", "nop", size,
              Benchmark(
                  [&]() {
                    for (uint64_t value : values) {
                      DoNotOptimize(value);
                    }
                    return 1;
                  },
                  size, 1, n_runs));
}

template <class HashSet>
size_t SwissMemoryEstimator(const HashSet& table) {
  return table.capacity() * (1 + sizeof(typename HashSet::value_type));
}

#endif  // HASH_BENCHMARK_H_
