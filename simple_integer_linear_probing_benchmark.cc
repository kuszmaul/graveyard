/* Benchmark lookups for the simplest ordered linear probing table: It
 * implemenets a set of integers with no vector instructions. */

#include <time.h>

#include <iostream>
#include <cstdint>

uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

int main() {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  std::cerr << "tdiff=" << (end-start) << "ns" << std::endl;
}
