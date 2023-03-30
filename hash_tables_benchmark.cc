#include <cstddef> // for size_t
#include <cstdint> // for uint64_t
#include <functional>  // for equal_to
#include <set>
#include <string>      // for string, basic_string
#include <type_traits> // for remove_reference_t
#include <utility>     // for pair
#include <vector>      // for vector

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/hash/hash.h"           // for Hash
#include "absl/strings/string_view.h" // for string_view
#include "enum_print.h"
#include "enums_flag.h"
#include "folly/container/F14Set.h"
#include "folly/lang/Bits.h" // for findLastSet
#include "graveyard_set.h"
#include "hash_benchmark.h"      // for IntHashSetBenchmark
#include "internal/hash_table.h" // for Buckets, HashTable<>::cons...
#include "ordered_linear_probing_set.h"

enum class Implementation {
  // Order these from most-important-to-benchmark to
  // least-important-to-benchmark.
  kGraveyard,
  kGoogle,
  kFacebook,
  kOLP,
  kGraveyardIdentityHash,
  kGoogleIdentityHash,
  kFacebookIdentityHash,
  kOLPIdentityHash,
  // Graveyard variants
  kGraveyard3578, // Fill the table to 7/8 then rehash to 3/5 full (instead of
                  // 3/4 full) to reduce number of rehashes.
  kGraveyard1278, // Fill the table to 7/8 then rehash to 1/2 full (instead of
                  // 3/4 full) to reduce number of rehashes.
  kGraveyard2345, // Fill the table to 4/5 then rehash to 2/3 full (instead of
                  // 3/4 full) to reduce number of rehashes.
  kGraveyard255,  // H2 computed modulo 255 (rather than 128)
};

namespace {
const auto *implementation_enum_and_strings =
    EnumsAndStrings<Implementation>::Create(
        {{Implementation::kGraveyard, "graveyard"},
         {Implementation::kGoogle, "google"},
         {Implementation::kFacebook, "facebook"},
         {Implementation::kOLP, "OLP"},
         {Implementation::kGraveyardIdentityHash, "graveyard-idhash"},
         {Implementation::kGoogleIdentityHash, "google-idhash"},
         {Implementation::kFacebookIdentityHash, "facebook-idhash"},
         {Implementation::kOLPIdentityHash, "OLP-idhash"},
         {Implementation::kGraveyard3578, "graveyard3578"},
         {Implementation::kGraveyard1278, "graveyard1278"},
         {Implementation::kGraveyard2345, "graveyard2345"},
         {Implementation::kGraveyard255, "graveyard255"}});
} // namespace

ABSL_FLAG(std::vector<Implementation>, implementations,
          implementation_enum_and_strings->Enums(),
          "comma-separated list of hash table implementations to benchmark");

std::string AbslUnparseFlag(std::vector<Implementation> implementations) {
  return AbslUnparseVectorEnumFlag(*implementation_enum_and_strings,
                                   implementations);
}

bool AbslParseFlag(std::string_view text,
                   std::vector<Implementation> *implementations,
                   std::string *error) {
  return AbslParseVectorEnumFlag(*implementation_enum_and_strings, text,
                                 implementations, error);
}

std::set<Implementation> FlaggedImplementations() {
  std::vector<Implementation> implementations_vector =
      absl::GetFlag(FLAGS_implementations);
  return std::set<Implementation>(implementations_vector.begin(),
                                  implementations_vector.end());
}

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

// GraveyardSet using 255 for the modulo
template <class Traits> class Traits255 : public Traits {
public:
  static constexpr size_t kH2Modulo = 255;
};
using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;
static_assert(Int64Traits::kH2Modulo == 128);
static_assert(Int64Traits::kSlotsPerBucket == 14);
using Int64Traits255 = Traits255<Int64Traits>;
static_assert(Int64Traits255::kH2Modulo == 255);
using Graveyard255 = yobiduck::internal::HashTable<Int64Traits255>;
static_assert(Int64Traits255::kSlotsPerBucket == 14);

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

template <class Traits> class Traits1278 : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 7;
  static constexpr size_t full_utilization_denominator = 8;
  static constexpr size_t rehashed_utilization_numerator = 1;
  static constexpr size_t rehashed_utilization_denominator = 2;
};
using Int64Traits1278 = Traits1278<Int64Traits>;
using Graveyard1278 = yobiduck::internal::HashTable<Int64Traits1278>;

template <class Traits> class Traits2345 : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 4;
  static constexpr size_t full_utilization_denominator = 5;
  static constexpr size_t rehashed_utilization_numerator = 2;
  static constexpr size_t rehashed_utilization_denominator = 3;
};
using Int64Traits2345 = Traits2345<Int64Traits>;
using Graveyard2345 = yobiduck::internal::HashTable<Int64Traits2345>;

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
  auto swiss_memory_estimator = [](const auto &table) {
    using table_type = std::remove_reference_t<decltype(table)>;
    return table.capacity() * (1 + sizeof(typename table_type::value_type));
  };
  for (Implementation implementation : FlaggedImplementations()) {
    switch (implementation) {
    case Implementation::kGraveyard: {
      using Graveyard = yobiduck::GraveyardSet<uint64_t>;
      IntHashSetBenchmark<Graveyard>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGraveyardIdentityHash: {
      using GraveyardNoHash = yobiduck::GraveyardSet<uint64_t, IdentityHash>;
      IntHashSetBenchmark<GraveyardNoHash>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGraveyard255: {
      IntHashSetBenchmark<Graveyard255>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGraveyard3578: {
      IntHashSetBenchmark<Graveyard3578>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGraveyard1278: {
      IntHashSetBenchmark<Graveyard1278>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGraveyard2345: {
      IntHashSetBenchmark<Graveyard2345>(
          Get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kOLP: {
      using OLP = OrderedLinearProbingSet<uint64_t>;
      IntHashSetBenchmark<OLP>(
          [](const OLP &table) { return table.memory_estimate(); },
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kOLPIdentityHash: {
      using OLPNoHash = OrderedLinearProbingSet<uint64_t, IdentityHash>;
      IntHashSetBenchmark<OLPNoHash>(
          // TODO: Use the facebook name for `memory_estimate()`.
          [](const OLPNoHash &table) { return table.memory_estimate(); },
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGoogle: {
      IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
          swiss_memory_estimator,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kGoogleIdentityHash: {
      using FlatNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
      IntHashSetBenchmark<FlatNoHash>(
          swiss_memory_estimator,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kFacebook: {
      using F14 = folly::F14FastSet<uint64_t>;
      IntHashSetBenchmark<F14>(
          get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    case Implementation::kFacebookIdentityHash: {
      using F14NoHash = folly::F14FastSet<uint64_t, IdentityHash>;
      IntHashSetBenchmark<F14NoHash>(
          get_allocated_memory_size,
          implementation_enum_and_strings->ToString(implementation));
      break;
    }
    }
  }
}
