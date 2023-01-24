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
  IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(N);
}
