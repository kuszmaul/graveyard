/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <time.h>

#include <iostream>
#include <cstdint>
#include <random>
#include <unordered_set>

#include "simple_integer_linear_probing.h"

uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

int main() {
  SimpleIntegerLinearProbing set;
  std::unordered_set<uint64_t> values;
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<uint64_t> uniform_dist;
  while (values.size() < 10) {
    values.insert(uniform_dist(e1));
  }
  for (uint64_t value : values) {
    set.insert(value);
  }
  for (uint64_t value : values) {
    assert(set.contains(value));
  }
  for (size_t i = 0; i < 10; ++i) {
    uint64_t value = uniform_dist(e1);
    if (values.find(value) == values.end()) {
      assert(!set.contains(value));
    }
  }
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  std::cerr << "tdiff=" << (end-start) << "ns" << std::endl;
}
