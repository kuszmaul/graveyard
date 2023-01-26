/* Benchmark swiss flat_hash_set table */

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "hash_benchmark.h"

int main() {
  absl::flat_hash_set<uint64_t> set;
  constexpr size_t N = 10000000;
  HashBenchmarkResults results;
  IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
      results, SwissMemoryEstimator<absl::flat_hash_set<uint64_t>>,
      "flat_hash_set", N);
  results.Print();
}
