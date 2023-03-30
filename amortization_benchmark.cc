/* Benchmark that shows the cost of rehashing.  We want to measure
 *   1. The time of the worst insert (the one that causes rehashing).
 *   2. Peak memory usage (which happens during rehashing.)
 *
 * The idea is that we'll improve those, so we want to know what the
 * competition is and to be able to see our improvements.
 */

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "enums_flag.h"
#include "hash_benchmark.h"

ABSL_FLAG(Implementation, implementation, Implementation::kGraveyard,
          "Which hash table implementation to benchmark");

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  switch (auto implementation = absl::GetFlag(FLAGS_implementation)) {
  default:
    LOG(INFO) << "No amortizaton benchmark for " << ImplementationString(implementation);
  }
}


