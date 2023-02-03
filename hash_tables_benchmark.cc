#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t
#include <vector>   // for vector

#include "absl/container/flat_hash_set.h"
#include "folly/container/F14Set.h"
#include "hash_benchmark.h"  // for IntHashSetBenchmark
#include "simple_integer_linear_probing.h"

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

int main() {
  IntHashSetBenchmark<SimpleIntegerLinearProbing>(
      [](const SimpleIntegerLinearProbing& table) {
        return table.memory_estimate();
      },
      "SimpleILP");
  IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
      SwissMemoryEstimator<absl::flat_hash_set<uint64_t>>, "flatset");
  using FlatNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
  IntHashSetBenchmark<FlatNoHash>(SwissMemoryEstimator<FlatNoHash>,
                                  "flatset-nohash");
  using F14 = folly::F14FastSet<uint64_t>;
  IntHashSetBenchmark<F14>(F14MemoryEstimator<F14>,
                           "F14");
}
