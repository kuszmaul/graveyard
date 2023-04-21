#ifndef BENCHMARK_TABLE_TYPES_H_
#define BENCHMARK_TABLE_TYPES_H_

#include "absl/container/flat_hash_set.h"
#include "folly/container/F14Set.h"
#include "graveyard_set.h"
#include "libcuckoo/cuckoohash_map.hh"
#include "ordered_linear_probing_set.h"

// Define the various table types used in the various benchmarks in
// the paper.

struct IdentityHash {
  size_t operator()(uint64_t v) const { return v; }
};

using GoogleSet = absl::flat_hash_set<uint64_t>;
using GoogleSetNoHash = absl::flat_hash_set<uint64_t, IdentityHash>;
using FacebookSet = folly::F14FastSet<uint64_t>;
using FacebookSetNoHash = folly::F14FastSet<uint64_t, IdentityHash>;
using CuckooSet = libcuckoo::cuckoohash_map<uint64_t, uint64_t>;

using Int64Traits =
    yobiduck::internal::HashTableTraits<uint64_t, void, absl::Hash<uint64_t>,
                                        std::equal_to<uint64_t>,
                                        std::allocator<uint64_t>>;

template <class Traits> class TraitsLikeAbseil : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 7;
  static constexpr size_t full_utilization_denominator = 8;
  static constexpr size_t rehashed_utilization_numerator = 7;
  static constexpr size_t rehashed_utilization_denominator = 16;
  static constexpr std::optional<size_t> kTombstonePeriod = std::nullopt;
};
using GraveyardLikeAbseil = yobiduck::internal::HashTable<TraitsLikeAbseil<Int64Traits>>;

using GraveyardLowLoad = GraveyardLikeAbseil;

template <class Traits> class TraitsMediumLoad : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 9;
  static constexpr size_t full_utilization_denominator = 10;
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 11;
  static constexpr std::optional<size_t> kTombstonePeriod = std::nullopt;
};
using GraveyardMediumLoad = yobiduck::internal::HashTable<TraitsMediumLoad<Int64Traits>>;

template <class Traits> class TraitsHighLoadNoGraveyard : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 925;
  static constexpr size_t full_utilization_denominator = 1000;
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 10;
  static constexpr std::optional<size_t> kTombstonePeriod = std::nullopt;
};
using GraveyardHighLoadNoGraveyard = yobiduck::internal::HashTable<TraitsHighLoadNoGraveyard<Int64Traits>>;

template <class Traits> class TraitsHighLoad : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 925;
  static constexpr size_t full_utilization_denominator = 1000;
  static constexpr size_t rehashed_utilization_numerator = 9;
  static constexpr size_t rehashed_utilization_denominator = 10;
  static constexpr std::optional<size_t> kTombstonePeriod = 20; // 5%
  static constexpr size_t kMaxExtraBuckets = 10;
};
using GraveyardHighLoad = yobiduck::internal::HashTable<TraitsHighLoad<Int64Traits>>;

template <class Traits> class TraitsVeryHighLoad : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 97;
  static constexpr size_t full_utilization_denominator = 100;
  static constexpr size_t rehashed_utilization_numerator = 96;
  static constexpr size_t rehashed_utilization_denominator = 100;
  static constexpr std::optional<size_t> kTombstonePeriod = 50; // 2 %
  static constexpr size_t kMaxExtraBuckets = 20;
};
using GraveyardVeryHighLoad = yobiduck::internal::HashTable<TraitsVeryHighLoad<Int64Traits>>;

struct NamePair {
  constexpr NamePair() {}
  constexpr NamePair(std::string_view human_v, std::string_view computer_v) :has_value(true), human(human_v), computer(computer_v) {}
  bool has_value = false;
  std::string_view human;
  std::string_view computer;
};

using OLPSet = OrderedLinearProbingSet<uint64_t>;
using OLPSetNoHash = OrderedLinearProbingSet<uint64_t, IdentityHash>;

template <class Table>
constexpr NamePair kTableNames = NamePair();

template<>
constexpr NamePair kTableNames<GoogleSet> = {"Google", "google"};
template<>
constexpr NamePair kTableNames<GoogleSetNoHash> = {"Google identity-hash", "google-idhash"};
template<>
constexpr NamePair kTableNames<FacebookSet> = {"Facebook", "facebook"};
template<>
constexpr NamePair kTableNames<FacebookSetNoHash> = {"Facebook identity-hash", "facebook-idhash"};
template<>
constexpr NamePair kTableNames<GraveyardLowLoad> = {"Graveyard low load", "graveyard-low-load"};
template<>
constexpr NamePair kTableNames<GraveyardMediumLoad> = {"Graveyard medium load", "graveyard-medium-load"};
template<>
constexpr NamePair kTableNames<GraveyardHighLoad> = {"Graveyard high load", "graveyard-high-load"};
template<>
constexpr NamePair kTableNames<GraveyardHighLoadNoGraveyard> = {"Graveyard high load, no graveyard tombstones", "graveyard-high-load-no-tombstones"};
template<>
constexpr NamePair kTableNames<GraveyardVeryHighLoad> = {"Graveyard very high load", "graveyard-very-high-load"};
template<>
constexpr NamePair kTableNames<OLPSet> = {"OLP", "OLP"};
template<>
constexpr NamePair kTableNames<OLPSetNoHash> = {"OLP identity-hash", "OLP-idhash"};
template<>
constexpr NamePair kTableNames<CuckooSet> = {"Cuckoo", "cuckoo"};

// Does the implementation provide a low high-water mark?
template <class Table>
constexpr std::optional<bool> kExpectLowHighWater = std::nullopt;
template<> constexpr std::optional<bool> kExpectLowHighWater<GoogleSet> = false;
template<> constexpr std::optional<bool> kExpectLowHighWater<FacebookSet> = false;
template<> constexpr std::optional<bool> kExpectLowHighWater<GraveyardLowLoad> = true;
template<> constexpr std::optional<bool> kExpectLowHighWater<GraveyardMediumLoad> = true;
template<> constexpr std::optional<bool> kExpectLowHighWater<GraveyardHighLoad> = true;
template<> constexpr std::optional<bool> kExpectLowHighWater<GraveyardVeryHighLoad> = true;

#endif  // BENCHMARK_TABLE_TYPES_H_
