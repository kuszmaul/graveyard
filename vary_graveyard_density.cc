#include "graveyard_set.h"

using yobiduck::GraveyardSet;
using yobiduck::internal::ProbeStatistics;

static size_t rehash_count = 0;

struct RehashCallback {
  template <class Table>
  void operator()(Table& table, size_t slot_count) {
    ProbeStatistics pre_stats = table.GetProbeStatistics();
    LOG(INFO) << "rehash: " << slot_count << " Probe stats: " << pre_stats.successful << " " << pre_stats.unsuccessful;
    LOG(INFO) << " size=" << table.size() << " capacity=" << table.capacity();
    ++rehash_count;
    table.rehash_internal(slot_count);
    ProbeStatistics post_stats = table.GetProbeStatistics();
    LOG(INFO) << "rehashed: " << slot_count << " Probe stats: " << post_stats.successful << " " << post_stats.unsuccessful;
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

  static constexpr std::optional<size_t> kTombstonePeriod = std::nullopt;
};
#endif

// Even with these parameters the probe lengths are hardly affected:
//  With:    rehash:   108112 Probe stats: 1.00328 1.04362
//           rehashed: 108112 Probe stats: 1.00719 1.08973
//  Without: rehash: 108112 Probe stats: 1.00325 1.04322
//           rehashed: 108112 Probe stats: 1.00041 1.00531
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

  static constexpr std::optional<size_t> kTombstonePeriod = std::nullopt;
};

using GraveyardInstrumented = yobiduck::internal::HashTable<LogRehashTraits<Int64Traits>>;

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
