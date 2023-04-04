/* Benchmark that shows the cost of rehashing.  We want to measure
 *   1. The time of the worst insert (the one that causes rehashing).
 *   2. Peak memory usage (which happens during rehashing.)
 *
 * The idea is that we'll improve those, so we want to know what the
 * competition is and to be able to see our improvements.
 *
 * These hash tables all have the property that for insertion-only
 * workloads, rehashing (to increase the size) occurs strictly as a
 * function of the number of elements in the table (or, if there is a
 * reserve(), as a function of the reserve sizes and the number of
 * insertions between each reserve).  So for each table, we can
 * determine at what size the table rehashes, and then, as a separate
 * run, provoke a rehash on a particular insertion.
 *
 * To determine what the constants are run with
 * `--find_rehash_points`.
 */

#include <sys/resource.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>
#include <time.h>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/hash/hash.h"                // for Hash
#include "absl/log/check.h"                // for GetReferenceableValue, CHE...
#include "absl/log/log.h"                  // for LogMessage, ABSL_LOGGING_I...
#include "absl/strings/str_cat.h"          // for StrCat
#include "absl/strings/string_view.h"      // for operator<<, string_view
#include "benchmark.h"                     // for GetTime, operator-
#include "folly/container/F14Set.h"
#include "graveyard_set.h"
#include "hash_benchmark.h"

// IWYU pragma: no_include <bits/types/struct_rusage.h>

ABSL_FLAG(bool, find_rehash_points, false,
	  "Instead of running the benchmark, find the number of insertions that cause a rehash to take place");

ABSL_FLAG(Implementation, implementation, Implementation::kGraveyard,
          "Which hash table implementation to benchmark");

using GoogleSet = absl::flat_hash_set<uint64_t>;
using FacebookSet = folly::F14FastSet<uint64_t>;
using GraveyardSet = yobiduck::GraveyardSet<uint64_t>;

template <class Table>
constexpr std::string_view kTableName = "unknown";

template<>
constexpr std::string_view kTableName<GoogleSet> = "GoogleSet";
template<>
constexpr std::string_view kTableName<FacebookSet> = "FacebookSet";
template<>
constexpr std::string_view kTableName<GraveyardSet> = "GraveyardSet";

// These magic numbers are gotten by running
// ```
// $ bazel-bin/amortization_benchmark --find_rehash_points
// ```
// and then copying the output to replace the following 6 lines.

template <class Table>
constexpr size_t kRehashPoint = 0;
// GoogleSet: rehashed at 117440512 from 134217727 to 268435455
template<> constexpr size_t kRehashPoint<GoogleSet> = 117440512;
// FacebookSet: rehashed at 100663296 from 117440512 to 234881024
template<> constexpr size_t kRehashPoint<FacebookSet> = 100663296;
// GraveyardSet: rehashed at 100000008 from 114285780 to 133333410
template<> constexpr size_t kRehashPoint<GraveyardSet> = 100000008;

// Does the implementation provide a low high-water mark?
template <class Table>
constexpr bool kExpectLowHighWater = 0;
template<> constexpr bool kExpectLowHighWater<GoogleSet> = false;
template<> constexpr bool kExpectLowHighWater<FacebookSet> = false;
template<> constexpr bool kExpectLowHighWater<GraveyardSet> = true;

struct MemoryStats {
  // All units are in KiloBytes
  size_t size, resident, shared, text, lib, data, dt; 
  size_t max_resident;
};

MemoryStats GetMemoryStats() {
  struct MemoryStats result;
  std::ifstream statfile("/proc/self/statm", std::ifstream::in);
  std::string data;
  std::getline(statfile, data);
  int n [[maybe_unused]] = sscanf(data.c_str(), "%lu %lu %lu %lu %lu %lu %lu",
		 &result.size, &result.resident, &result.shared, &result.text, &result.lib, &result.data, &result.dt);
  assert(n == 7);
  result.size *= 4;
  result.resident *= 4;
  result.shared *= 4;
  result.text *= 4;
  result.lib *= 4;
  result.dt *= 4;
  struct rusage r_usage;
  getrusage(RUSAGE_SELF, &r_usage);
  result.max_resident = r_usage.ru_maxrss;
  return result;
}

void ResetMaxRss() {
  std::ofstream clear_refs("/proc/self/clear_refs", std::ofstream::out);
  clear_refs << "5" << std::endl;
}

template <class Table>
void AmortizationBenchmark(std::string_view implementation) {
  MemoryStats before = GetMemoryStats();
  MemoryStats after;
  {
    Table t;
    for (size_t i = 0; i < 10'000'000; ++i) {
      t.insert(i);
      if (false) {
	if ((i & (i - 1)) == 0) {
	  MemoryStats here= GetMemoryStats();
	  std::cout << "here: " << here.resident << " " << here.max_resident << std::endl;
	}
      }
    }
    after = GetMemoryStats();
  }
  std::cout << "#Implementation\tResidentKiB\tMaxRSSKiB" << std::endl;
  std::cout << implementation << "\t" << after.resident - before.resident << "\t" << after.max_resident - before.max_resident << std::endl;
  std::cerr << "before: " << before.resident << " " << before.max_resident << std::endl;
  std::cout << "after:  " << after.resident << " "  << after.max_resident << std::endl;
  MemoryStats fin= GetMemoryStats();
  std::cout << "fin:     " << fin.resident << " " << fin.max_resident << std::endl;
  ResetMaxRss();
  MemoryStats resetted= GetMemoryStats();
  std::cout << "reset:   " << resetted.resident << " " << resetted.max_resident << std::endl;
}

static constexpr size_t kMinimumSize = 100'000'000;

template <class Table>
void FindRehashPoints() {
  Table s;
  s.reserve(kMinimumSize);
  for (size_t i = 0; i < kMinimumSize; ++i) {
    s.insert(i);
  }
  size_t starting_capacity = s.capacity();
  for (size_t i = kMinimumSize; true; ++i) {
    s.insert(i);
    if (s.capacity() != starting_capacity) {
      std::cout << "// " << kTableName<Table> << ": rehashed at " << i << " from " << starting_capacity << " to " << s.capacity() << std::endl;
      std::cout << "template<> constexpr size_t kRehashPoint<" << kTableName<Table> << "> = " << i << ";" << std::endl;
      break;
    }
  }
}

std::string DurationString(uint64_t time_in_ns) {
  if (time_in_ns < 1000) {
    return absl::StrCat(time_in_ns, "ns");
  } else if (time_in_ns < 1'000'000) {
    return absl::StrCat(time_in_ns/1000.0, "us");
  } else if (time_in_ns < 1'000'000'000) {
    return absl::StrCat(time_in_ns/1000.0/1000.0, "ms");
  } else {
    return absl::StrCat(time_in_ns/1000.0/1000.0/1000.0, "s");
  }
}

template <class Table>
void MeasureRehash() {
  // We measure the memory statistics several times:
  //
  // Before doing anything
  MemoryStats memory_at_beginning = GetMemoryStats();
  // Just before doing an insert that doesn't rehash (a "noncritical
  // insert").
  MemoryStats memory_before_noncritical_insert;
  // Just after the noncritical insert before we do an insert that
  // rehashes.
  MemoryStats memory_before_critical_insert;
  // After the critical insert.
  MemoryStats memory_after_critical_insert;
  timespec start_fast, end_fast, start_slow, end_slow;
  {
    Table s;
    s.reserve(kMinimumSize);
    size_t i = 0 ;
    for (; i < kMinimumSize; ++i) {
      s.insert(i);
    }
    size_t initial_capacity = s.capacity();
    for (; i < kRehashPoint<Table> - 1; ++i) {
      s.insert(i);
    }
    // This rehash shouldn't take long
    memory_before_noncritical_insert = GetMemoryStats();
    start_fast = GetTime();
    s.insert(i);
    end_fast = GetTime();
    ++i;
    // This is the rehash that takes a long time
    size_t just_before_rehash = s.capacity();
    start_slow = GetTime();
    memory_before_critical_insert = GetMemoryStats();
    s.insert(i);
    end_slow = GetTime();
    size_t just_after_rehash = s.capacity();
    CHECK_EQ(initial_capacity, just_before_rehash);
    CHECK_LT(just_before_rehash, just_after_rehash);
    memory_after_critical_insert = GetMemoryStats();
    auto ratio = [](double a, double b) { return a / b; };
    if (kExpectLowHighWater<Table>) {
      // For the graveyard table, the critical rehash shouldn't have used much memory.
      //
      // The max didn't get much bigger.
      CHECK_LT(ratio(memory_after_critical_insert.max_resident, memory_after_critical_insert.resident), 1.05);
      // The resident memory didn't grow much in the critical insert.
      CHECK_LT(ratio(memory_after_critical_insert.resident , memory_before_critical_insert.resident), 1.2);
    } else {
      // For the non-graveyard tables, the critical rehash uses nearly
      // 3/2 as much memory.
      CHECK_GT(ratio(memory_after_critical_insert.max_resident, memory_after_critical_insert.resident), 1.45);
      // And the resident memory grows by nearly a factor of two.
      CHECK_GT(ratio(memory_after_critical_insert.resident , memory_before_critical_insert.resident), 1.85);
    }
  } 
  ResetMaxRss();
  MemoryStats resetted = GetMemoryStats();
  uint64_t fast_time = end_fast - start_fast;
  uint64_t slow_time = end_slow - start_slow;
  LOG(INFO) << kTableName<Table>;
  LOG(INFO) << " Fast: " << DurationString(fast_time) << " Slow: " << DurationString(slow_time) << " ratio=" << static_cast<double>(slow_time) / fast_time;
  auto show_memory = [](std::string when, const MemoryStats& stats) {
    constexpr size_t kWidth = 20;
    if (when.size() < kWidth) {
      when.append(kWidth - when.size(), ' ');
    }
    LOG(INFO) << " " << when << " rss = " << std::setw(7) << stats.resident << " maxrss = " << std::setw(7) << stats.max_resident << " ratio=" << stats.max_resident * 1.0 / stats.resident;
  };
  show_memory("Beginning:", memory_at_beginning);
  show_memory("Before Noncritical:", memory_before_noncritical_insert);
  show_memory("Before critical:", memory_before_critical_insert);
  show_memory("After critical:", memory_after_critical_insert);
  show_memory("After destruction:", resetted);
}


int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  if (absl::GetFlag(FLAGS_find_rehash_points)) {
    std::cout << "Measuring" << std::endl;
    FindRehashPoints<GoogleSet>();
    FindRehashPoints<FacebookSet>();
    FindRehashPoints<GraveyardSet>();
  } else {
    MeasureRehash<GoogleSet>();
    MeasureRehash<FacebookSet>();
    MeasureRehash<GraveyardSet>();
  }
}
