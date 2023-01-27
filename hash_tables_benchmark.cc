#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "hash_benchmark.h"  // for IntHashSetBenchmark
#include "simple_integer_linear_probing.h"

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

int main() {
  constexpr size_t N = 10'000'000;
  HashBenchmarkResults results;
  for (size_t i = N / 2; i < N; i += N / 20) {
    size_t n = i + N / 20;
    std::cerr << "Working on n=" << n << std::endl;
    IntHashSetBenchmark<SimpleIntegerLinearProbing>(
        results,
        [](const SimpleIntegerLinearProbing& table) {
          return table.memory_estimate();
        },
        "SimpleILP", n);
    IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
        results, SwissMemoryEstimator<absl::flat_hash_set<uint64_t>>,
        "flatset", n);
    using FlatNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
    IntHashSetBenchmark<FlatNoHash>(
        results, SwissMemoryEstimator<FlatNoHash>,
        "flatset-nohash", n);
  }
  // results.Print();
  results.Print2(N);
}
