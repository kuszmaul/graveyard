#include "graveyard_map.h"

#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Pair;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

TEST(GraveyardMap, Types) {
  using IntSet = yobiduck::GraveyardMap<uint64_t, std::string>;
  using value_type = std::pair<const uint64_t, std::string>;
  static_assert(std::is_same_v<IntSet::value_type, value_type>);
  static_assert(std::is_same_v<IntSet::key_type, uint64_t>);
  static_assert(std::is_same_v<IntSet::size_type, size_t>);
  static_assert(std::is_same_v<IntSet::difference_type, ptrdiff_t>);
  static_assert(
      std::is_same_v<IntSet::hasher,
                     absl::container_internal::hash_default_hash<uint64_t>>);
  static_assert(
      std::is_same_v<IntSet::key_equal,
                     absl::container_internal::hash_default_eq<uint64_t>>);
  static_assert(
      std::is_same_v<IntSet::allocator_type, std::allocator<value_type>>);
  static_assert(std::is_same_v<IntSet::reference, value_type &>);
  static_assert(std::is_same_v<IntSet::const_reference, const value_type &>);
  static_assert(std::is_same_v<IntSet::pointer, value_type *>);
  static_assert(std::is_same_v<IntSet::const_pointer, const value_type *>);
}

TEST(GraveyardSet, Basic) {
  yobiduck::GraveyardMap<uint64_t, std::string> map;
  EXPECT_EQ(map.size(), 0ul);
  EXPECT_TRUE(!map.contains(0l));
  EXPECT_THAT(map.insert({0ul, "a"}), Pair(_, true));
  EXPECT_TRUE(map.contains(0l));
  EXPECT_TRUE(!map.contains(1l));
}

TEST(GraveyardMap, EmptyIterator) {
  yobiduck::GraveyardMap<uint64_t, std::string> map;
  EXPECT_TRUE(map.begin() == map.end());
}

TEST(GraveyardMap, EmptyConstIterator) {
  yobiduck::GraveyardMap<uint64_t, std::string> map;
  EXPECT_TRUE(map.cbegin() == map.cend());
  const yobiduck::GraveyardMap<uint64_t, std::string> *cmap = &map;
  EXPECT_TRUE(cmap->cbegin() == cmap->cend());
  EXPECT_TRUE(cmap->begin() == cmap->end());
  EXPECT_TRUE(cmap->cbegin() == cmap->end());
  EXPECT_TRUE(cmap->begin() == cmap->cend());
  EXPECT_TRUE(map.begin() == cmap->cbegin());
  EXPECT_TRUE(map.end() == cmap->cend());
}

TEST(GraveyardSet, IteratorOneElement) {
  yobiduck::GraveyardMap<uint64_t, std::string> map;
  EXPECT_THAT(map.insert({100ul, std::string("a")}), Pair(_, true));
  EXPECT_TRUE(map.contains(100ul));
  yobiduck::GraveyardMap<uint64_t, std::string>::iterator it;
  it = map.begin();
  EXPECT_TRUE(it != map.end());
  ++it;
  CHECK(it == map.end());
  EXPECT_THAT(map, UnorderedElementsAre(Pair(100ul, "a")));
}

TEST(GraveyardMap, RandomInserts) {
  absl::BitGen bitgen;
  const size_t j = 1000;
  yobiduck::GraveyardMap<uint64_t, std::string> map;
  absl::flat_hash_map<uint64_t, std::string> fmap;
  for (size_t i = 0; i < j; ++i) {
    {
      size_t v = absl::Uniform<uint64_t>(bitgen);
      std::pair<uint64_t, std::string> pair = {v, absl::StrCat(v)};
      map.insert(pair);
      fmap.insert(pair);
    }
    // We don't yet have the power to check that map is a submap of fmap.
    if (i < 10 || i % 60 == 0) {
      EXPECT_THAT(map, UnorderedElementsAreArray(fmap));
    }
  }
  EXPECT_THAT(map, UnorderedElementsAreArray(fmap));
}

TEST(GraveyardMap, Reserve) {
  yobiduck::GraveyardMap<uint64_t, uint64_t> map;
  map.reserve(1000);
  map.insert({100u, 200u});
  EXPECT_EQ(map.size(), 1);
}

TEST(GraveyardMap, AssignAndReserve) {
  yobiduck::GraveyardMap<uint64_t, uint64_t> map;
  for (size_t i = 1000; i < 2000; ++i) {
    map = yobiduck::GraveyardMap<uint64_t, uint64_t>();
    map.reserve(1000);
  }
}

#if 0
  IntSet::allocator_type allocator;
  LOG(INFO) << "Allocator type: " << typeid(allocator).name();
  int status;
  char *demangled = abi::__cxa_demangle(typeid(allocator).name(), 0, 0, &status);
  CHECK_EQ(status, 0);
  LOG(INFO) << "Demangled type: " << demangled;
#endif
