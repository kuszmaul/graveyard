// Measures the probe lengths under a hovering workload.

#include <cstddef>
#include <cstdint>
#include <functional> // for equal_to
#include <memory>     // for allocator

#include "absl/hash/hash.h" // for Hash
#include "absl/log/log.h"   // for LogMessage, ABSL_LOGGING_INTERNAL_LOG_INFO
#include "graveyard_set.h"

static size_t rehash_count = 0;

struct NoteRehashCallback {
  template <class Table> void operator()(Table &table, size_t slot_count) {
    ++rehash_count;
    table.rehash_internal(slot_count);
  }
};

using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;

template <class Traits> class NoteRehashTraits : public Traits {
public:
  using rehash_callback = NoteRehashCallback;
};

using GraveyardInstrumented =
    yobiduck::internal::HashTable<NoteRehashTraits<Int64Traits>>;

// 90% full after rehash
template <class Traits> class NoteRehashTraits90 : public Traits {
public:
  using rehash_callback = NoteRehashCallback;

  // 95 percent load when full
  static constexpr size_t full_utilization_numerator = 19;
  static constexpr size_t full_utilization_denominator = 20;
  // 90.0% percent load after rehash
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 10;

  // Tombstone one every 42.
  static_assert(Traits::kSlotsPerBucket == 14);
  static constexpr yobiduck::internal::TombstoneRatio kTombstoneRatio{1, 3};
};

using GraveyardInstrumented90 =
    yobiduck::internal::HashTable<NoteRehashTraits90<Int64Traits>>;

// TODO: Turn these traits classes into structs and get rid of the `public:`.

template <class Traits> class NoGraveyard : public NoteRehashTraits90<Traits> {
public:
  static constexpr yobiduck::internal::TombstoneRatio kTombstoneRatio{};
};

using NoGraveyardInstrumented90 =
    yobiduck::internal::HashTable<NoGraveyard<NoteRehashTraits90<Int64Traits>>>;

template <class Table> void Hover() {
  constexpr size_t kN = 10'000'000;
  Table set(kN);
  for (size_t i = 0; i < kN; ++i) {
    set.insert(i);
  }
  size_t next_to_erase = 0;
  size_t next_to_insert = set.size();
  LOG(INFO) << "size=" << set.size();
  size_t i;
  for (i = 0; i < kN; ++i) {
    // for (i = 0; i < 10*kN; ++i) {
    if (i % (kN / 10) == 0) {
      auto [successful, unsuccessful, insert] = set.GetProbeStatistics();
      LOG(INFO) << i << " s=" << successful << " u=" << unsuccessful
                << " i=" << insert;
    }
    set.erase(next_to_erase++);
    set.insert(next_to_insert++);
  }
  auto [successful, unsuccessful, insert] = set.GetProbeStatistics();
  LOG(INFO) << i << " s=" << successful << " u=" << unsuccessful
            << " i=" << insert;
}

int main() {
  LOG(INFO) << "rehash at 7/8";
  Hover<GraveyardInstrumented>();
  LOG(INFO) << "rehash at 95% to 90% graveyard=42";
  Hover<GraveyardInstrumented90>();
  LOG(INFO) << "rehash at 95% to 90% nograveyard";
  Hover<NoGraveyardInstrumented90>();
}
