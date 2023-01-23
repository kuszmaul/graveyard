/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <cstdint>
#include <random>
#include <unordered_set>

#include "benchmark.h"
#include "simple_integer_linear_probing.h"

int main() {
  SimpleIntegerLinearProbing set;
  std::unordered_set<uint64_t> values;
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<uint64_t> uniform_dist;
  constexpr size_t N = 10000000;
  while (values.size() < N) {
    values.insert(uniform_dist(e1));
  }
  Benchmark(
      [&]() {
        for (uint64_t value : values) {
          set.insert(value);
        }
        return values.size();
      },
      "insert", "insertion");
  Benchmark(
      [&]() {
        for (uint64_t value : values) {
          bool contains = set.contains(value);
          assert(contains);
          DoNotOptimize(contains);
        }
        return values.size();
      },
      "contains", "found");
  std::unordered_set<uint64_t> not_values;
  while (not_values.size() < N) {
    not_values.insert(uniform_dist(e1));
  }
  Benchmark(
      [&]() {
        for (uint64_t value : not_values) {
          bool contains = set.contains(value);
          assert(!contains);
          DoNotOptimize(contains);
        }
        return not_values.size();
      },
      "!contains", "not_found");
  Benchmark([]() { return 1; }, "1 nothing", "nothing");
}
