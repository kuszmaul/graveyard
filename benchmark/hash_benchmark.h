#ifndef HASH_BENCHMARK_H_
#define HASH_BENCHMARK_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream> // for operator<<, basic_ostream, basic_ostream<>...
#include <set>
#include <string>
#include <string_view> // for string_view
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "benchmark.h"
#include "benchmark/table_types.h" // IWYU pragma: keep
#include "enum_print.h"

// IWYU note: kTableNames, which is a templated constexpr variable,
// doesn't work right with IWYU, so we have to keep table_types.h.

ABSL_DECLARE_FLAG(size_t, size_growth);
enum class Operation { kInsert, kReservedInsert, kFound, kNotFound };
ABSL_DECLARE_FLAG(absl::flat_hash_set<Operation>, operations);

enum class Implementation {
  // Order these from most-important-to-benchmark to
  // least-important-to-benchmark.
  kGraveyardLow,
  kGraveyardMedium,
  kGraveyardHighLoad, // Fill to 92.5% full then hash to 90% full with 5%
                      // graveyard tombstones,
  kGraveyardHighLoadNoGraveyard, // Same as HighLoad, except no
                                 // graveyard tombstones.
  kGraveyardVeryHighLoad,
  kGoogle,
  kFacebook,
  kOLP,
  kGraveyardIdentityHash,
  kGoogleIdentityHash,
  kFacebookIdentityHash,
  kOLPIdentityHash,
// Graveyard variants
#if 0
  kGraveyard3578, // Fill the table to 7/8 then rehash to 3/5 full (instead of
                  // 3/4 full) to reduce number of rehashes.
#endif
  kGraveyardLikeAbseil, // Fill the table to 7/8 then rehash to 7/16 full with
                        // no graveyard tombstones.
#if 0
  kGraveyard2345, // Fill the table to 4/5 then rehash to 2/3 full (instead of
                  // 3/4 full) to reduce number of rehashes.
#endif
  kLibCuckoo,
};
// TODO: Make this a set, not an unordered set.
ABSL_DECLARE_FLAG(absl::flat_hash_set<Implementation>, implementations);

extern const EnumsAndStrings<Implementation> *implementation_enum_and_strings;

bool AbslParseFlag(std::string_view text, Implementation *implementation,
                   std::string *error);

std::string AbslUnparseFlag(Implementation implementation);

// Return the --implementation
std::set<Implementation> FlaggedImplementations();

// These functions don't return a flat-hash-set since the sort order will be
// correlated and make the flat hash set look much faster than it is.  The
// returned vector is full or random integers, in a random order.
//
// Returns a set of distinct number of cardinality `size`.  Returns the same set
// if we run this several times within a single process (but the set may be
// different from run-to-run).
void GetSomeNumbers(size_t size, std::vector<uint64_t> &result);
// Return a set of distinct numbers of cardinality `size`, but it doesn't
// interesect `GetSomeNumbers(size)`.  Return the same set within several runs
// in the same process.
std::vector<uint64_t>
GetSomeOtherNumbers(const std::vector<uint64_t> &other_numbers);

bool OperationIsFlagged(Operation operation);

std::string FileNameForHashSetBenchmark(Operation operation,
                                        std::string_view implementation);

template <class HashSet>
void IntHashSetBenchmark(
    std::function<size_t(const HashSet &)> memory_estimator) {
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
  static_assert(kTableNames<HashSet>.has_value);
  auto implementation = kTableNames<HashSet>;
  LOG(INFO) << implementation.human;
  // TODO: Make "insert" contant into kConstant.
  if (Operation op = Operation::kInsert; OperationIsFlagged(op)) {
    std::ofstream output(
        FileNameForHashSetBenchmark(op, implementation.computer),
        std::ios::out);
    CHECK(output.is_open());
    Benchmark(
        output,
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
  if (Operation op = Operation::kReservedInsert; OperationIsFlagged(op)) {
    std::ofstream output(
        FileNameForHashSetBenchmark(op, implementation.computer),
        std::ios::out);
    CHECK(output.is_open());
    Benchmark(
        output,
        [&](size_t size, size_t trial) {
          if (trial == 0) {
            GetSomeNumbers(size, values);
          }
          set = HashSet();
          if (0)
            std::cerr << "Reserving " << size << std::endl;
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

  if (Operation op = Operation::kFound; OperationIsFlagged(op)) {
    std::ofstream output(
        FileNameForHashSetBenchmark(op, implementation.computer),
        std::ios::out);
    CHECK(output.is_open());
    Benchmark(
        output,
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
  if (Operation op = Operation::kNotFound; OperationIsFlagged(op)) {
    std::ofstream output(
        FileNameForHashSetBenchmark(op, implementation.computer),
        std::ios::out);
    CHECK(output.is_open());
    std::vector<uint64_t> other_numbers;
    Benchmark(
        output,
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

#endif // HASH_BENCHMARK_H_
