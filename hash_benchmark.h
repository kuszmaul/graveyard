#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
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

class HashBenchmarkResult {
 public:
  void Add(std::string_view implementation, std::string_view operation,
           size_t size, BenchmarkResult result, size_t final_memory) {
    results[Key{.implementation = std::string(implementation),
                .operation = std::string(operation),
                .size = size}]
        .push_back({.benchmark_result = std::move(result),
                    .final_memory = final_memory});
  }
  void Print() const {
    for (const auto& [key, result_vector] : results) {
      for (const Result& result : result_vector) {
        std::cout << key.implementation << ": "
                  << result.benchmark_result.Mean() << "±"
                  << result.benchmark_result.StandardDeviation() * 2 << "ns/"
                  << key.operation << " size=" << key.size << std::endl;
      }
    }
  }

 private:
  struct Key {
    std::string implementation;
    std::string operation;
    size_t size;

   private:
    friend bool operator<(const Key& a, const Key& b) {
      if (a.implementation < b.implementation) return true;
      if (b.implementation < a.implementation) return false;
      if (a.operation < b.operation) return true;
      if (b.operation < a.operation) return false;
      return a.size < b.size;
    }
  };
  struct Result {
    BenchmarkResult benchmark_result;
    size_t final_memory;
  };
  std::map<Key, std::vector<Result>> results;
};

template <class HashSet>
HashBenchmarkResult IntHashSetBenchmark(std::string_view implementation,
                                        size_t size) {
  const auto& values = GetSomeNumbers(size);
  HashBenchmarkResult results;
  results.Add(implementation, "insert", size, Benchmark([&]() {
                HashSet set;
                for (uint64_t value : values) {
                  set.insert(value);
                }
                return values.size();
              }),
              0);
  HashSet set;
  for (uint64_t value : values) {
    set.insert(value);
  }
  results.Add(implementation, "contains(true)", size, Benchmark([&]() {
                for (uint64_t value : values) {
                  bool contains = set.contains(value);
                  assert(contains);
                  DoNotOptimize(contains);
                }
                return values.size();
              }),
              0);
  const auto& not_values = GetSomeOtherNumbers(size);
  results.Add(implementation, "contains(false)", size, Benchmark([&]() {
                for (uint64_t value : not_values) {
                  bool contains = set.contains(value);
                  assert(!contains);
                  DoNotOptimize(contains);
                }
                return not_values.size();
              }),
              0);
  results.Add("nop", "nop", size, Benchmark([&]() {
                for (uint64_t value : values) {
                  DoNotOptimize(value);
                }
                return values.size();
              }),
              0);
  return results;
}

#endif  // HASH_BENCHMARK_H_
