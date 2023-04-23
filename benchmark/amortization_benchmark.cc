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
#include <initializer_list> // for initializer_list
#include <iomanip>
#include <iostream>
#include <string>
#include <time.h>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"           // for GetReferenceableValue, CHE...
#include "absl/log/log.h"             // for LogMessage, ABSL_LOGGING_I...
#include "absl/strings/str_cat.h"     // for StrCat
#include "absl/strings/str_format.h"  // for StrCat
#include "absl/strings/string_view.h" // for operator<<, string_view
#include "benchmark.h"                // for GetTime, operator-
#include "benchmark/print_numbers.h"
#include "benchmark/table_types.h"
#include "graveyard_set.h"

// IWYU pragma: no_include <bits/types/struct_rusage.h>

template <class Table> constexpr size_t rehash_point = 0;

// These numbers are actually computed in the benchmark.

// GoogleSet: rehashed at 117440512 from 134217727 to 268435455
template <> size_t rehash_point<GoogleSet>;
// FacebookSet: rehashed at 100663296 from 117440512 to 234881024
template <> size_t rehash_point<FacebookSet>;
// Graveyard low load: rehashed at 100000008 from 114285794 to 228571532
template <> size_t rehash_point<GraveyardLowLoad>;
// Graveyard medium load: rehashed at 100000000 from 111111182 to 122222296
template <> size_t rehash_point<GraveyardMediumLoad>;
// Graveyard high load: rehashed at 100000010 from 103092864 to 104166762
template <> size_t rehash_point<GraveyardHighLoad>;
// Graveyard very high Load: rehashed at 100000010 from 103092864 to 104166762
template <> size_t rehash_point<GraveyardVeryHighLoad>;

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
  int n [[maybe_unused]] =
      sscanf(data.c_str(), "%lu %lu %lu %lu %lu %lu %lu", &result.size,
             &result.resident, &result.shared, &result.text, &result.lib,
             &result.data, &result.dt);
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
          MemoryStats here = GetMemoryStats();
          std::cout << "here: " << here.resident << " " << here.max_resident
                    << std::endl;
        }
      }
    }
    after = GetMemoryStats();
  }
  std::cout << "#Implementation\tResidentKiB\tMaxRSSKiB" << std::endl;
  std::cout << implementation << "\t" << after.resident - before.resident
            << "\t" << after.max_resident - before.max_resident << std::endl;
  std::cerr << "before: " << before.resident << " " << before.max_resident
            << std::endl;
  std::cout << "after:  " << after.resident << " " << after.max_resident
            << std::endl;
  MemoryStats fin = GetMemoryStats();
  std::cout << "fin:     " << fin.resident << " " << fin.max_resident
            << std::endl;
  ResetMaxRss();
  MemoryStats resetted = GetMemoryStats();
  std::cout << "reset:   " << resetted.resident << " " << resetted.max_resident
            << std::endl;
}

static constexpr size_t kMinimumSize = 100'000'000;

template <class Table> void FindRehashPoints() {
  Table s;
  s.reserve(kMinimumSize);
  for (size_t i = 0; i < kMinimumSize; ++i) {
    s.insert(i);
  }
  size_t starting_capacity = s.capacity();
  for (size_t i = kMinimumSize; true; ++i) {
    s.insert(i);
    if (s.capacity() != starting_capacity) {
      rehash_point<Table> = i;
      static_assert(kTableNames<Table>.has_value);
      std::cout << "// " << kTableNames<Table>.human << ": rehashed at " << i
                << " from " << starting_capacity << " to " << s.capacity()
                << std::endl;
      break;
    }
  }
}

constexpr size_t kKilo = 1000;
constexpr size_t kMega = kKilo * kKilo;
constexpr size_t kGiga = kKilo * kMega;

std::string DurationString(uint64_t time_in_ns) {
  if (time_in_ns < kKilo) {
    return absl::StrCat(time_in_ns, "ns");
  } else if (time_in_ns < kMega) {
    return absl::StrCat(time_in_ns / double(kKilo), "us");
  } else if (time_in_ns < kGiga) {
    return absl::StrCat(time_in_ns / double(kMega), "ms");
  } else {
    return absl::StrCat(time_in_ns / double(kGiga), "s");
  }
}

std::string NumberWithSuffix(uint64_t n, size_t digits) {
  if (n >= kGiga) {
    LOG(FATAL) << "Not ready";
  }
  if (n >= 100 * kMega) {
    CHECK(digits >= 3);
    return absl::StrFormat("%*.*fM", digits, digits - 3, n / double(kMega));
  } else {
    LOG(FATAL) << "Not ready for size " << n;
  }
}

template <class Table> void MeasureRehash(std::ofstream &ofile) {
  // We measure the memory statistics several times:
  //
  // Before doing anything
  MemoryStats memory_at_beginning = GetMemoryStats();
  // Just after the noncritical insert before we do an insert that
  // rehashes.
  MemoryStats memory_before_critical_insert;
  // After the critical insert.
  MemoryStats memory_after_critical_insert;
  timespec start_slow, end_slow;
  // capacities:
  size_t just_before_rehash, just_after_rehash;
  static_assert(kTableNames<Table>.has_value);
  {
    Table s;
    s.reserve(kMinimumSize);
    size_t i = 0;
    for (; i < kMinimumSize; ++i) {
      s.insert(i);
    }
    size_t initial_capacity = s.capacity();
    for (; i < rehash_point<Table>; ++i) {
      s.insert(i);
    }
    // This is the rehash that takes a long time
    LOG(INFO) << "Doing i=" << i;
    just_before_rehash = s.capacity();
    LOG(INFO) << "before cap=" << s.capacity();
    start_slow = GetTime();
    memory_before_critical_insert = GetMemoryStats();
    s.insert(i);
    end_slow = GetTime();
    just_after_rehash = s.capacity();
    LOG(INFO) << "after cap=" << s.capacity();
    CHECK_EQ(initial_capacity, just_before_rehash)
        << " Expected rehash (" << kTableNames<Table>.human << ") at "
        << rehash_point<Table>;
    CHECK_LT(just_before_rehash, just_after_rehash);
    memory_after_critical_insert = GetMemoryStats();
    auto ratio = [](double a, double b) { return a / b; };
    static_assert(kExpectLowHighWater<Table>.has_value());
    if (*kExpectLowHighWater<Table>) {
      // For the graveyard table, the critical rehash shouldn't have used much
      // memory.
      //
      // The max didn't get much bigger.
      CHECK_LT(ratio(memory_after_critical_insert.max_resident,
                     memory_after_critical_insert.resident),
               1.05);
      // The resident memory grows by a factor of two (since we are
      // using parameters that "like abseil" double the table.
      CHECK_LT(ratio(memory_after_critical_insert.resident,
                     memory_before_critical_insert.resident),
               2); // 1.2);
    } else {
      // For the non-graveyard tables, the critical rehash uses nearly
      // 3/2 as much memory.
      CHECK_GT(ratio(memory_after_critical_insert.max_resident,
                     memory_after_critical_insert.resident),
               1.45);
      // And the resident memory grows by nearly a factor of two.
      CHECK_GT(ratio(memory_after_critical_insert.resident,
                     memory_before_critical_insert.resident),
               1.85);
    }
  }
  ResetMaxRss();
  MemoryStats resetted = GetMemoryStats();
  // Implementation
  ofile << absl::StrFormat("%-21s", kTableNames<Table>.human) << " & ";
  // Rehash at
  ofile << NumberWithSuffix(rehash_point<Table>, 4) << " & ";
  // Rehash multiplier
  ofile << absl::StrFormat("%.3f       & ", double(just_after_rehash) /
                                                double(just_before_rehash));
  LOG(INFO) << kTableNames<Table>.human;
  auto show_memory = [](std::string when, const MemoryStats &stats) {
    constexpr size_t kWidth = 20;
    if (when.size() < kWidth) {
      when.append(kWidth - when.size(), ' ');
    }
    LOG(INFO) << " " << when << " rss = " << std::setw(7) << stats.resident
              << " maxrss = " << std::setw(7) << stats.max_resident
              << " ratio=" << stats.max_resident * 1.0 / stats.resident;
  };
  show_memory("Beginning:", memory_at_beginning);
  show_memory("Before critical:", memory_before_critical_insert);
  show_memory("After critical:", memory_after_critical_insert);
  show_memory("After destruction:", resetted);
  // RSS before
  ofile << absl::StrFormat(
      "%4sMiB & ",
      PrintWithPrecision(memory_before_critical_insert.resident / 1024.0, 3));
  // RSS After
  ofile << absl::StrFormat(
      "%4sMib & ",
      PrintWithPrecision(memory_after_critical_insert.resident / 1024.0, 3));
  // RSS ratio
  ofile << absl::StrFormat("%4.2f  & ",
                           memory_after_critical_insert.resident /
                               double(memory_before_critical_insert.resident));
  // high-water
  ofile << absl::StrFormat(
      "%-4sMiB    & ",
      PrintWithPrecision(memory_after_critical_insert.max_resident / 1024.0,
                         3));
  // high-water ratio
  ofile << absl::StrFormat("%4.2f       \\\\",
                           memory_after_critical_insert.max_resident /
                               double(memory_after_critical_insert.resident));
  ofile << std::endl;
}

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  LOG(INFO) << "Finding rehash points";
  FindRehashPoints<GoogleSet>();
  FindRehashPoints<FacebookSet>();
  FindRehashPoints<GraveyardLowLoad>();
  FindRehashPoints<GraveyardMediumLoad>();
  FindRehashPoints<GraveyardHighLoad>();
  FindRehashPoints<GraveyardVeryHighLoad>();

  LOG(INFO) << "Measuring";
  std::ofstream ofile;
  ofile.open("/home/bradley/github/graveyard/paper/experiments/rss.tex");
  ofile << "\\begin{center}" << std::endl;
  ofile << "\\begin{tabular}{llllllll}" << std::endl;
  ofile << "Implementation        & Rehash & Rehash      & RSS     & RSS     & "
           "RSS   & High-water & High-water \\\\"
        << std::endl;
  ofile << "                      & at     & multipler   & before  & after   & "
           "ratio &            & ratio      \\\\ \\hline"
        << std::endl;
  MeasureRehash<GoogleSet>(ofile);
  MeasureRehash<FacebookSet>(ofile);
  MeasureRehash<GraveyardLowLoad>(ofile);
  MeasureRehash<GraveyardMediumLoad>(ofile);
  MeasureRehash<GraveyardHighLoad>(ofile);
  MeasureRehash<GraveyardVeryHighLoad>(ofile);
  ofile << "\\end{tabular}" << std::endl;
  ofile << "\\end{center}" << std::endl;
}
