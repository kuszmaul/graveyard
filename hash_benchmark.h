#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>     // for operator<<, basic_ostream, basic_ostream<>...
#include <string_view>  // for string_view
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"  // for StrCat
#include "benchmark.h"

ABSL_DECLARE_FLAG(size_t, size_growth);
ABSL_DECLARE_FLAG(std::vector<std::string>, operations);

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

absl::flat_hash_set<std::string>& GetOperations();

template <class HashSet>
void IntHashSetBenchmark(std::function<size_t(const HashSet&)> memory_estimator,
                         std::string_view implementation) {
  size_t size_growth = absl::GetFlag(FLAGS_size_growth);
  std::vector<size_t> sizes;
  for (size_t size = 1; size < size_growth; ++size) {
    sizes.push_back(size);
  }
  for (size_t size = size_growth; size < 10'000'000;
       size += size / size_growth) {
    sizes.push_back(size);
  }
  // LOG(INFO) << "Opening " << absl::StrCat(implementation, ".data");
  HashSet set;
  std::vector<uint64_t> values;
  LOG(INFO) << implementation;
  // TODO: Make "insert" contant into kConstant.
  if (GetOperations().contains("insert")) {
    std::ofstream insert_output(
        absl::StrCat("data/insert_", implementation, ".data"), std::ios::out);
    CHECK(insert_output.is_open());
    Benchmark(
        insert_output,
        [&](size_t size, size_t trial) {
          if (trial == 0) {
            GetSomeNumbers(size, values);
          }
          set = HashSet();
        },
        [&]() {
          for (uint64_t value : values) {
            set.insert(value);
          }
          return memory_estimator(set);
        },
        sizes);
  }
  if (GetOperations().contains("reserved-insert")) {
    std::ofstream reserved_insert_output(
        absl::StrCat("data/reserved-insert_", implementation, ".data"),
        std::ios::out);
    CHECK(reserved_insert_output.is_open());
    Benchmark(
        reserved_insert_output,
        [&](size_t size, size_t trial) {
          if (trial == 0) {
            GetSomeNumbers(size, values);
          }
          set = HashSet();
          if (0) std::cerr << "Reserving " << size << std::endl;
          set.reserve(size);
        },
        [&]() {
          for (uint64_t value : values) {
            set.insert(value);
          }
          return memory_estimator(set);
        },
        sizes);
  }

  if (GetOperations().contains("found")) {
    std::ofstream found_output(
        absl::StrCat("data/found_", implementation, ".data"), std::ios::out);
    CHECK(found_output.is_open());
    Benchmark(
        found_output,
        [&](size_t size, size_t trial) {
          if (trial == 0) {
            set = HashSet();
            GetSomeNumbers(size, values);
            for (uint64_t value : values) {
              set.insert(value);
            }
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
  }
  if (GetOperations().contains("notfound")) {
    std::ofstream notfound_output(
        absl::StrCat("data/notfound_", implementation, ".data"), std::ios::out);
    CHECK(notfound_output.is_open());
    std::vector<uint64_t> other_numbers;
    Benchmark(
        notfound_output,
        [&](size_t size, size_t trial) {
          if (trial == 0) {
            set = HashSet();
            GetSomeNumbers(size, values);
            other_numbers = GetSomeOtherNumbers(values);
            for (uint64_t value : values) {
              set.insert(value);
            }
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
}

template <class HashSet>
size_t SwissMemoryEstimator(const HashSet& table) {
  return table.capacity() * (1 + sizeof(typename HashSet::value_type));
}

template <class HashSet>
size_t F14MemoryEstimator(const HashSet& table) {
  return table.getAllocatedMemorySize();
}

#endif  // HASH_BENCHMARK_H_
