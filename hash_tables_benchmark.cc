#include <algorithm>  // for max
#include <cstddef>    // for size_t
#include <cstdint>    // for uint64_t
#include <string>     // for string, basic_string
#include <vector>     // for vector

#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/string_view.h"  // for string_view
#include "folly/container/F14Set.h"
#include "folly/lang/Bits.h"  // for findLastSet
#include "hash_benchmark.h"   // for IntHashSetBenchmark
#include "ordered_linear_probing_set.h"

ABSL_FLAG(std::vector<std::string>, tables,
          std::vector<std::string>({"OLP", "OLP-idhash", "google",
                                    "google-idhash", "facebook",
                                    "facebook-idhash"}),
          "comma-separated list of hash table implementations to benchmark");

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  absl::flat_hash_set<std::string> tables;
  {
    std::vector<std::string> tables_vector = absl::GetFlag(FLAGS_tables);
    tables = absl::flat_hash_set<std::string>(tables_vector.begin(),
                                              tables_vector.end());
  }
  if (constexpr absl::string_view name = "OLP";
      tables.contains(name)) {
    using OLP = OrderedLinearProbingSet<uint64_t>;
    IntHashSetBenchmark<OLP>(
        [](const OLP& table) { return table.memory_estimate(); }, name);
  }
  if (constexpr absl::string_view name = "OLP-idhash";
      tables.contains(name)) {
    using OLPNoHash = OrderedLinearProbingSet<uint64_t, IdentityHash>;
    IntHashSetBenchmark<OLPNoHash>(
        // TODO: Use the facebook name for `memory_estimate()`.
        [](const OLPNoHash& table) { return table.memory_estimate(); },
        name);
  }
  if (constexpr absl::string_view name = "google";
      tables.contains(name)) {
    IntHashSetBenchmark<absl::flat_hash_set<uint64_t>>(
        SwissMemoryEstimator<absl::flat_hash_set<uint64_t>>, name);
  }
  if (constexpr absl::string_view name = "google-idhash";
      tables.contains(name)) {
    using FlatNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
    IntHashSetBenchmark<FlatNoHash>(SwissMemoryEstimator<FlatNoHash>,
                                    name);
  }
  if (constexpr absl::string_view name = "facebook-idhash";
      tables.contains(name)) {
    using F14 = folly::F14FastSet<uint64_t>;
    IntHashSetBenchmark<F14>(F14MemoryEstimator<F14>, name);
  }
  if (constexpr absl::string_view name = "facebook";
      tables.contains(name)) {
    using F14NoHash = folly::F14FastSet<uint64_t, IdentityHash>;
    IntHashSetBenchmark<F14NoHash>(F14MemoryEstimator<F14NoHash>, name);
  }
}
