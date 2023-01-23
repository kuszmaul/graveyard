/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <cstdint>
#include <unordered_set>

#include "benchmark.h"
#include "simple_integer_linear_probing.h"

int main() {
  SimpleIntegerLinearProbing set;
  std::unordered_set<uint64_t> values;
  constexpr size_t N = 10000000;
  IntHashSetBenchmark<SimpleIntegerLinearProbing>(N);
}
