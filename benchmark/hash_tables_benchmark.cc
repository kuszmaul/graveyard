#include <cstddef>     // for size_t
#include <cstdint>     // for uint64_t
#include <functional>  // for equal_to
#include <type_traits> // for remove_reference_t
#include <vector>      // for vector

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h" // for GetFlag
#include "absl/flags/parse.h"
#include "absl/hash/hash.h"           // for Hash
#include "benchmark/hash_benchmark.h" // for IntHashSetBenchmark
#include "benchmark/table_types.h"
#include "folly/container/F14Set.h"
#include "folly/lang/Bits.h" // for findLastSet
#include "graveyard_set.h"
#include "libcuckoo/cuckoohash_map.hh"

using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;

#if 0
// GraveyardSet is 3/4 to 7/8 by default
// This one is 3/5 to 7/8 to reduce the number of rehashes
template <class Traits> class Traits3578 : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 7;
  static constexpr size_t full_utilization_denominator = 8;
  static constexpr size_t rehashed_utilization_numerator = 3;
  static constexpr size_t rehashed_utilization_denominator = 5;
};
using Int64Traits3578 = Traits3578<Int64Traits>;
using Graveyard3578 = yobiduck::internal::HashTable<Int64Traits3578>;
#endif

template <class Traits> class Traits2345 : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 4;
  static constexpr size_t full_utilization_denominator = 5;
  static constexpr size_t rehashed_utilization_numerator = 2;
  static constexpr size_t rehashed_utilization_denominator = 3;
};
using Int64Traits2345 = Traits2345<Int64Traits>;
using Graveyard2345 = yobiduck::internal::HashTable<Int64Traits2345>;

template <class Traits> class Traits9092 : public Traits {
public:
  // 92.5 percent load when full
  static constexpr size_t full_utilization_numerator = 37;
  static constexpr size_t full_utilization_denominator = 40;
  // 90.0% percent load after rehash
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 10;
};
using Int64Traits9092 = Traits9092<Int64Traits>;
using Graveyard9092 = yobiduck::internal::HashTable<Int64Traits9092>;

template <class Traits>
class Traits9092NoGraveyard : public Traits9092<Traits> {
public:
  static constexpr yobiduck::internal::TombstoneRatio kTombstoneRatio{};
};
using Int64Traits9092NoGraveyard = Traits9092NoGraveyard<Int64Traits>;
using Graveyard9092NoGraveyard =
    yobiduck::internal::HashTable<Int64Traits9092NoGraveyard>;

using GraveyardNoHash = yobiduck::GraveyardSet<uint64_t, IdentityHash>;

template <>
constexpr NamePair kTableNames<GraveyardNoHash> = {"graveyard identity-hash",
                                                   "graveyard-idhash"};

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  // The difference between these two is the 'g' vs. the 'G'.  The lower-case
  // 'g' is for F14. The upper-case 'G' is for google-style naming used in
  // `GraveyardSet`.
  auto Get_allocated_memory_size = [](const auto &table) {
    return table.GetAllocatedMemorySize();
  };
  auto get_allocated_memory_size = [](const auto &table) {
    return table.getAllocatedMemorySize();
  };
  auto cuckoo_allocated_memory_size = [](const auto &table) {
    return table.capacity() * (1 + // for the partial_t
                               1 + // for the occupied
                               // Don't penalize libcuckoo for not
                               // implementating a set, so just count
                               // the size of the key.
                               sizeof(uint64_t));
  };
  auto swiss_memory_estimator = [](const auto &table) {
    using table_type = std::remove_reference_t<decltype(table)>;
    return table.capacity() * (1 + sizeof(typename table_type::value_type));
  };
  for (Implementation implementation : absl::GetFlag(FLAGS_implementations)) {
    switch (implementation) {
    case Implementation::kGraveyardLow: {
      IntHashSetBenchmark<GraveyardLowLoad>(Get_allocated_memory_size);
      break;
    }
    case Implementation::kGraveyardMedium: {
      IntHashSetBenchmark<GraveyardMediumLoad>(Get_allocated_memory_size);
      break;
    }
    case Implementation::kGraveyardHighLoad: {
      IntHashSetBenchmark<GraveyardHighLoad>(Get_allocated_memory_size);
      break;
    }
    case Implementation::kGraveyardHighLoadNoGraveyard: {
      IntHashSetBenchmark<GraveyardHighLoadNoGraveyard>(
          Get_allocated_memory_size);
      break;
    }
    case Implementation::kGraveyardVeryHighLoad: {
      IntHashSetBenchmark<GraveyardVeryHighLoad>(Get_allocated_memory_size);
      break;
    }
    case Implementation::kGraveyardIdentityHash: {
      IntHashSetBenchmark<GraveyardNoHash>(Get_allocated_memory_size);
      break;
    }
#if 0
    case Implementation::kGraveyard3578: {
      IntHashSetBenchmark<Graveyard3578>(Get_allocated_memory_size);
      break;
    }
#endif
    case Implementation::kGraveyardLikeAbseil: {
      IntHashSetBenchmark<GraveyardLikeAbseil>(Get_allocated_memory_size);
      break;
    }
#if 0
    case Implementation::kGraveyard2345: {
      IntHashSetBenchmark<Graveyard2345>(Get_allocated_memory_size);
      break;
    }
#endif
    case Implementation::kOLP: {
      IntHashSetBenchmark<OLPSet>(
          [](const OLPSet &table) { return table.memory_estimate(); });
      break;
    }
    case Implementation::kOLPIdentityHash: {
      IntHashSetBenchmark<OLPSetNoHash>(
          // TODO: Use the facebook name for `memory_estimate()`.
          [](const OLPSetNoHash &table) { return table.memory_estimate(); });
      break;
    }
    case Implementation::kGoogle: {
      IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
          swiss_memory_estimator);
      break;
    }
    case Implementation::kGoogleIdentityHash: {
      IntHashSetBenchmark<GoogleSetNoHash>(swiss_memory_estimator);
      break;
    }
    case Implementation::kFacebook: {
      using F14 = folly::F14FastSet<uint64_t>;
      IntHashSetBenchmark<F14>(get_allocated_memory_size);
      break;
    }
    case Implementation::kFacebookIdentityHash: {
      IntHashSetBenchmark<FacebookSetNoHash>(get_allocated_memory_size);
      break;
    }
    case Implementation::kLibCuckoo: {
      IntHashSetBenchmark<CuckooSet>(cuckoo_allocated_memory_size);
      break;
    }
    }
  }
}
