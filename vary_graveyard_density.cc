#include "graveyard_set.h"

using yobiduck::GraveyardSet;
using yobiduck::internal::ProbeStatistics;

struct RehashCallback {
  template <class Table>
  void operator()(Table& table, size_t slot_count) {
    ProbeStatistics stats = table.GetProbeStatistics();
    LOG(INFO) << "rehash: " << slot_count << " Probe stats: " << stats.successful << " " << stats.unsuccessful;
    table.rehash_internal(slot_count);
  }
};

using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;

template <class Traits> class LogRehashTraits : public Traits {
 public:
  using rehash_callback = RehashCallback;
};
using GraveyardInstrumented = yobiduck::internal::HashTable<LogRehashTraits<Int64Traits>>;

int main() {
  constexpr size_t kN = 10'000;
  GraveyardInstrumented set(kN);
  for (size_t i = 0; i < kN; ++i) {
    set.insert(i);
  }
  for (size_t i = kN; i < kN + kN / 10; ++i) {
    set.insert(i);
  }
}
