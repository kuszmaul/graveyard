#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>

#include "absl/hash/hash.h" // for Hash
#include "graveyard_set.h"
#include "internal/hash_table.h" // for ProbeStatistics, Buckets

int main(int argc, char *argv[]) {
  constexpr size_t kSize = 10'000'000;
  yobiduck::GraveyardSet<uint64_t> set(kSize);
  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<uint64_t> uniform_dist;
  for (size_t i = 0; i < kSize * 7 / 8; ++i) {
    set.insert(uniform_dist(e1));
  }
  auto [success, unsuccess] = set.GetProbeStatistics();
  std::cout << "Success   = " << success << std::endl;
  std::cout << "Unsuccess = " << unsuccess << std::endl;
}
