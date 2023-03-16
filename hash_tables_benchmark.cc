#include <algorithm>  // for max
#include <cstddef>    // for size_t
#include <cstdint>    // for uint64_t
#include <string>     // for string, basic_string
#include <utility>    // for pair
#include <vector>     // for vector

#include "absl/algorithm/container.h"  // for c_find, ContainerIter
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/string_view.h"  // for string_view
#include "enum_print.h"
#include "enums_flag.h"
#include "folly/container/F14Set.h"
#include "folly/lang/Bits.h"  // for findLastSet
#include "graveyard_set.h"
#include "hash_benchmark.h"  // for IntHashSetBenchmark
#include "ordered_linear_probing_set.h"

enum class Implementation {
  kOLP,
  kOLPIdentityHash,
  kGraveyard,
  kGraveyardIdentityHash,
  kGoogle,
  kGoogleIdentityHash,
  kFacebook,
  kFacebookIdentityHash
};

namespace {
const auto* implementation_enum_and_strings =
    EnumsAndStrings<Implementation>::Create(
        {{Implementation::kGraveyard, "graveyard"},
         {Implementation::kGraveyardIdentityHash, "graveyard-idhash"},
         {Implementation::kOLP, "OLP"},
         {Implementation::kOLPIdentityHash, "OLP-idhash"},
         {Implementation::kGoogle, "google"},
         {Implementation::kGoogleIdentityHash, "google-idhash"},
         {Implementation::kFacebook, "facebook"},
         {Implementation::kFacebookIdentityHash, "facebook-idhash"}});
}  // namespace

ABSL_FLAG(std::vector<Implementation>, implementations,
          implementation_enum_and_strings->Enums(),
          "comma-separated list of hash table implementations to benchmark");

std::string AbslUnparseFlag(std::vector<Implementation> implementations) {
  return AbslUnparseVectorEnumFlag(*implementation_enum_and_strings,
                                   implementations);
}

bool AbslParseFlag(std::string_view text,
                   std::vector<Implementation>* implementations,
                   std::string* error) {
  return AbslParseVectorEnumFlag(*implementation_enum_and_strings, text,
                                 implementations, error);
}

bool ImplementationIsFlagged(Implementation implementation) {
  std::vector<Implementation> implementations_vector =
      absl::GetFlag(FLAGS_implementations);
  return absl::c_find(implementations_vector, implementation) !=
         implementations_vector.end();
}

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  // The difference between these two is the 'g' vs. the 'G'.  The lower-case
  // 'g' is for F14. The upper-case 'G' is for google-style naming used in
  // `GraveyardSet`.
  auto Get_allocated_memory_size = [](const auto& table) {
    return table.GetAllocatedMemorySize();
  };
  auto get_allocated_memory_size = [](const auto& table) {
    return table.getAllocatedMemorySize();
  };
  auto swiss_memory_estimator = [](const auto& table) {
    using table_type = std::remove_reference_t<decltype(table)>;
    return table.capacity() * (1 + sizeof(typename table_type::value_type));
  };
  if (const auto implementation = Implementation::kGraveyard;
      ImplementationIsFlagged(implementation)) {
    using Graveyard = yobiduck::GraveyardSet<uint64_t>;
    IntHashSetBenchmark<Graveyard>(
        Get_allocated_memory_size,
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kGraveyardIdentityHash;
      ImplementationIsFlagged(implementation)) {
    using GraveyardNoHash = yobiduck::GraveyardSet<uint64_t, IdentityHash>;
    IntHashSetBenchmark<GraveyardNoHash>(
        Get_allocated_memory_size,
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kOLP;
      ImplementationIsFlagged(implementation)) {
    using OLP = OrderedLinearProbingSet<uint64_t>;
    IntHashSetBenchmark<OLP>(
        [](const OLP& table) { return table.memory_estimate(); },
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kOLPIdentityHash;
      ImplementationIsFlagged(implementation)) {
    using OLPNoHash = OrderedLinearProbingSet<uint64_t, IdentityHash>;
    IntHashSetBenchmark<OLPNoHash>(
        // TODO: Use the facebook name for `memory_estimate()`.
        [](const OLPNoHash& table) { return table.memory_estimate(); },
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kGoogle;
      ImplementationIsFlagged(implementation)) {
    IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
        swiss_memory_estimator,
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kGoogleIdentityHash;
      ImplementationIsFlagged(implementation)) {
    using FlatNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
    IntHashSetBenchmark<FlatNoHash>(
        swiss_memory_estimator,
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kFacebook;
      ImplementationIsFlagged(implementation)) {
    using F14 = folly::F14FastSet<uint64_t>;
    IntHashSetBenchmark<F14>(
        get_allocated_memory_size,
        implementation_enum_and_strings->ToString(implementation));
  }
  if (const auto implementation = Implementation::kFacebookIdentityHash;
      ImplementationIsFlagged(implementation)) {
    using F14NoHash = folly::F14FastSet<uint64_t, IdentityHash>;
    IntHashSetBenchmark<F14NoHash>(
        get_allocated_memory_size,
        implementation_enum_and_strings->ToString(implementation));
  }
}
