#include <cassert>    // for assert
#include <cstddef>    // for size_t
#include <cstdint>    // for uint64_t
#include <functional> // for equal_to
#include <memory>     // for allocator

#include "absl/hash/hash.h" // for Hash
#include "absl/log/log.h"   // for LogMessage, ABSL_LOGGING_INTERNAL_LOG_INFO
#include "graveyard_set.h"  // for Buckets, ProbeStatistics, HashTable, Gra...

using yobiduck::GraveyardSet;
using yobiduck::internal::ProbeStatistics;

static size_t rehash_count = 0;

struct RehashCallback {
  template <class Table> void operator()(Table &table, size_t slot_count) {
    ProbeStatistics pre_stats = table.GetProbeStatistics();
    LOG(INFO) << "rehash: " << slot_count
              << " Probe stats: s=" << pre_stats.successful
              << " u=" << pre_stats.unsuccessful << " i=" << pre_stats.insert;
    LOG(INFO) << " size=" << table.size() << " capacity=" << table.capacity();
    ++rehash_count;
    table.rehash_internal(slot_count);
    ProbeStatistics post_stats = table.GetProbeStatistics();
    LOG(INFO) << "rehashed: " << slot_count
              << " Probe stats: s=" << post_stats.successful
              << " u=" << post_stats.unsuccessful << " i=" << post_stats.insert;
    LOG(INFO) << " size=" << table.size() << " capacity=" << table.capacity();
  }
};

using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;

#if 0
// The tombstone period makes almost no difference for these parameters (need to measure insertion cost)
//  With tombstones:    rehashed: 111126 Probe stats: 1.00317 1.03956
//  Without tombstones: rehashed: 111126 Probe stats: 1.00009 1.00113
template <class Traits> class LogRehashTraits : public Traits {
 public:
  using rehash_callback = RehashCallback;

  // 92.5 percent load when full
  static constexpr size_t full_utilization_numerator = 37;
  static constexpr size_t full_utilization_denominator = 40;
  // 90.0% percent load after rehash
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 10;

  static constexpr yobiduck::internal::TombstoneRatio kTombstoneRatio{};
};
#endif

// Even with these relatively-high load parameters the probe lengths
// are hardly affected.  (Removing the tombstones hurts lookups a
// little and helps inserts a little).
//
//  With(40): rehash:   108112 Probe stats: s=1.00331 u=1.04402 i=1.49781
//            rehashed: 108112 Probe stats: s=1.00494 u=1.06215 i=1.13816
//  Without:  rehash:   108112 Probe stats: s=1.00344 u=1.04575 i=1.49169
//            rehashed: 108112 Probe stats: s=1.00035 u=1.00453 i=1.16833
//
// ("rehash" is before rehash.  rehashed is after.  Note that
// rehashing with tombstones increased the table size and increased
// the probe lengths.  So it does make a slight difference.  As
// expected, without tombstones, rehashing to a bigger table reduces
// probe length.)
//
// Need to measure the insertion cost.

template <class Traits> class LogRehashTraits : public Traits {
public:
  using rehash_callback = RehashCallback;

  // 95% load when full
  static constexpr size_t full_utilization_numerator = 19;
  static constexpr size_t full_utilization_denominator = 20;
  // 92.5% percent load after rehash
  static constexpr size_t rehashed_utilization_numerator = 37;
  static constexpr size_t rehashed_utilization_denominator = 40;

  // Tombstone one in 40 slots.  Therefore the ratio is 7/20 (in 280
  // slots, which 20 buckets, we want 7 tombstone so that 280/7 = 40.
  static_assert(Traits::kSlotsPerBucket == 14);
  static constexpr yobiduck::internal::TombstoneRatio kTombstoneRatio{7, 20};
};

using GraveyardInstrumented =
    yobiduck::internal::HashTable<LogRehashTraits<Int64Traits>>;

int main() {
  constexpr size_t kN = 100000;
  GraveyardInstrumented set(kN);
  for (size_t i = 0; i < kN; ++i) {
    set.insert(i);
  }
  assert(rehash_count == 1);
  for (size_t i = kN; rehash_count == 1; ++i) {
    set.insert(i);
  }
}
