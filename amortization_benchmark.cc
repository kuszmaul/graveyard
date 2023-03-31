/* Benchmark that shows the cost of rehashing.  We want to measure
 *   1. The time of the worst insert (the one that causes rehashing).
 *   2. Peak memory usage (which happens during rehashing.)
 *
 * The idea is that we'll improve those, so we want to know what the
 * competition is and to be able to see our improvements.
 */

#include <sys/resource.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"                  // for LogMessage, ABSL_LOGGING_I...
#include "absl/strings/string_view.h"      // for operator<<, string_view
#include "folly/container/F14Set.h"
#include "graveyard_set.h"
#include "hash_benchmark.h"

// IWYU pragma: no_include <bits/types/struct_rusage.h>

ABSL_FLAG(Implementation, implementation, Implementation::kGraveyard,
          "Which hash table implementation to benchmark");

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

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  switch (auto implementation = absl::GetFlag(FLAGS_implementation)) {
  case Implementation::kGoogle: {
    AmortizationBenchmark<absl::flat_hash_set<uint64_t>>("google");
    break;
  }
  case Implementation::kFacebook: {
    AmortizationBenchmark<folly::F14FastSet<uint64_t>>("facebook");
    break;
  }
  case Implementation::kGraveyard: {
    AmortizationBenchmark<yobiduck::GraveyardSet<uint64_t>>("graveyard");
    break;
  }
  default:
    LOG(INFO) << "No amortizaton benchmark for " << ImplementationString(implementation);
  }
}

