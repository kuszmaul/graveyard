/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <time.h>

#include <iostream>
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
  Benchmark([&]() {
    for (uint64_t value : values) {
      set.insert(value);
    }
    return values.size();
  },
    "insert", "insertion");
  {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t value : values) {
      bool contains = set.contains(value);
      assert(contains);
      DoNotOptimize(contains);
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    std::cerr << "contains=" << (end-start)/double(N) << "ns/found" << std::endl;
  }
  std::unordered_set<uint64_t> not_values;
  while (not_values.size() < N) {
    not_values.insert(uniform_dist(e1));
  }
  {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t value : not_values) {
      bool contains = set.contains(value);
      assert(!contains);
      DoNotOptimize(contains);
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    std::cerr << "!contains=" << (end-start)/double(N) << "ns/not_found" << std::endl;
  }
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  std::cerr << "nothing=" << (end-start) << "ns" << std::endl;
}
