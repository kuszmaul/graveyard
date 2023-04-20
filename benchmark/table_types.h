#ifndef BENCHMARK_TABLE_TYPES_H_
#define BENCHMARK_TABLE_TYPES_H_

#include "absl/container/flat_hash_set.h"
#include "folly/container/F14Set.h"
#include "graveyard_set.h"
#include "libcuckoo/cuckoohash_map.hh"

// Define the various table types used in the various benchmarks in
// the paper.

using GoogleSet = absl::flat_hash_set<uint64_t>;
using FacebookSet = folly::F14FastSet<uint64_t>;
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
  static constexpr std::optional<size_t> kTombstonePeriod = 40; // 2.5%
};
using GraveyardHighLoad = yobiduck::internal::HashTable<TraitsHighLoad<Int64Traits>>;

template <class Traits> class TraitsVeryHighLoad : public Traits {
public:
  static constexpr size_t full_utilization_numerator = 97;
  static constexpr size_t full_utilization_denominator = 100;
  static constexpr size_t rehashed_utilization_numerator = 96;
  static constexpr size_t rehashed_utilization_denominator = 100;
  static constexpr std::optional<size_t> kTombstonePeriod = 50; // 2 %
};
using GraveyardVeryHighLoad = yobiduck::internal::HashTable<TraitsVeryHighLoad<Int64Traits>>;

template <class Table>
constexpr std::optional<std::string_view> kTableName = std::nullopt;

template<>
constexpr std::optional<std::string_view> kTableName<GoogleSet> = "Google";
template<>
constexpr std::optional<std::string_view> kTableName<FacebookSet> = "Facebook";
template<>
constexpr std::optional<std::string_view> kTableName<GraveyardLowLoad> = "Graveyard low load";
template<>
constexpr std::optional<std::string_view> kTableName<GraveyardMediumLoad> = "Graveyard medium load";
template<>
constexpr std::optional<std::string_view> kTableName<GraveyardHighLoad> = "Graveyard high Load";
template<>
constexpr std::optional<std::string_view> kTableName<GraveyardVeryHighLoad> = "Graveyard very high Load";

template <class Table>
constexpr size_t rehash_point = 0;

// These numbers are actually computed in the benchmark.

// GoogleSet: rehashed at 117440512 from 134217727 to 268435455
template<> size_t rehash_point<GoogleSet>;
// FacebookSet: rehashed at 100663296 from 117440512 to 234881024
template<> size_t rehash_point<FacebookSet>;
// Graveyard low load: rehashed at 100000008 from 114285794 to 228571532
template<> size_t rehash_point<GraveyardLowLoad>;
// Graveyard medium load: rehashed at 100000000 from 111111182 to 122222296
template<> size_t rehash_point<GraveyardMediumLoad>;
// Graveyard high load: rehashed at 100000010 from 103092864 to 104166762
template<> size_t rehash_point<GraveyardHighLoad>;
// Graveyard very high Load: rehashed at 100000010 from 103092864 to 104166762
template<> size_t rehash_point<GraveyardVeryHighLoad>;

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
