#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>     // for operator<<, basic_ostream, basic_ostream<>...
#include <string_view>  // for string_view
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"  // for StrCat
#include "benchmark.h"

// These functions don't return a flat-hash-set since the sort order will be
// correlated and make the flat hash set look much faster than it is.  The
// returned vector is full or random integers, in a random order.
//
// Returns a set of distinct number of cardinality `size`.  Returns the same set
// if we run this several times within a single process (but the set may be
// different from run-to-run).
void GetSomeNumbers(size_t size, std::vector<uint64_t>& result);
// Return a set of distinct numbers of cardinality `size`, but it doesn't
// interesect `GetSomeNumbers(size)`.  Return the same set within several runs
// in the same process.
std::vector<uint64_t> GetSomeOtherNumbers(
    const std::vector<uint64_t>& other_numbers);

template <class HashSet>
void IntHashSetBenchmark(std::function<size_t(const HashSet&)> memory_estimator,
                         std::string_view implementation) {
  std::vector<size_t> sizes;
  for (size_t size = 1; size < 200; ++size) {
    sizes.push_back(size);
  }
  for (size_t size = 200; size < 10'000'000; size += size / 100) {
    sizes.push_back(size);
  }
  //LOG(INFO) << "Opening " << absl::StrCat(implementation, ".data");
  std::ofstream insert_output(absl::StrCat("data/insert_", implementation, ".data"), std::ios::out);
  CHECK(insert_output.is_open());
  std::ofstream reserved_insert_output(absl::StrCat("data/reserved-insert_", implementation, ".data"), std::ios::out);
  CHECK(reserved_insert_output.is_open());
  std::ofstream found_output(absl::StrCat("data/found_", implementation, ".data"), std::ios::out);
  CHECK(found_output.is_open());
  std::ofstream notfound_output(absl::StrCat("data/notfound_", implementation, ".data"), std::ios::out);
  CHECK(notfound_output.is_open());
  HashSet set;
  std::vector<uint64_t> values;
  Benchmark(
      insert_output,
      [&](size_t size) {
        set = HashSet();
        GetSomeNumbers(size, values);
      },
      [&]() {
        for (uint64_t value : values) {
          set.insert(value);
        }
        return memory_estimator(set);
      },
      sizes);


  Benchmark(
      reserved_insert_output,
      [&](size_t size) {
        set = HashSet();
        set.reserve(size);
        GetSomeNumbers(size, values);
      },
      [&]() {
        for (uint64_t value : values) {
          set.insert(value);
        }
        return memory_estimator(set);
      },
      sizes);

  Benchmark(
      found_output,
      [&](size_t size) {
        set = HashSet();
        GetSomeNumbers(size, values);
        for (uint64_t value : values) {
          set.insert(value);
        }
      },
      [&]() {
        for (uint64_t value : values) {
          bool contains = set.contains(value);
          assert(contains);
          DoNotOptimize(contains);
        }
        return memory_estimator(set);
      },
      sizes);
  std::vector<uint64_t> other_numbers;
  Benchmark(
      notfound_output,
      [&](size_t size) {
        set = HashSet();
        GetSomeNumbers(size, values);
        other_numbers = GetSomeOtherNumbers(values);
        for (uint64_t value : values) {
          set.insert(value);
        }
      },
      [&]() {
        for (uint64_t value : other_numbers) {
          bool contains = set.contains(value);
          assert(!contains);
          DoNotOptimize(contains);
        }
        return memory_estimator(set);
      },
      sizes);
}

template <class HashSet>
size_t SwissMemoryEstimator(const HashSet& table) {
  return table.capacity() * (1 + sizeof(typename HashSet::value_type));
}

#endif  // HASH_BENCHMARK_H_
