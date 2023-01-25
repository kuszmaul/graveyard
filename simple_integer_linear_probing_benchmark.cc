/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "hash_benchmark.h"  // for IntHashSetBenchmark
#include "simple_integer_linear_probing.h"

int main() {
  constexpr size_t N = 10000000;
  HashBenchmarkResults results;
  IntHashSetBenchmark<SimpleIntegerLinearProbing>(
      results,
      [](const SimpleIntegerLinearProbing &table) {
        return table.memory_estimate();
      },
      "SimpleIntegerLinearProbing", N);
  results.Print();
}
