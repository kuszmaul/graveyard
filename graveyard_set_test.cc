#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "graveyard_set.h"

using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

TEST(GraveyardSet, Basic) {
  yobiduck::GraveyardSet<uint64_t> set;
  CHECK_EQ(set.size(), 0ul);
  CHECK(!set.contains(0l));
  CHECK(set.insert(0ul));
  CHECK(set.contains(0l));
  CHECK(!set.contains(1l));
}

TEST(GraveyardSet, EmptyIterator) {
  yobiduck::GraveyardSet<uint64_t> set;
  CHECK(set.begin() == set.end());
}

TEST(GraveyardSet, EmptyConstIterator) {
  yobiduck::GraveyardSet<uint64_t> set;
  CHECK(set.cbegin() == set.cend());
  const yobiduck::GraveyardSet<uint64_t>* cset = &set;
  CHECK(cset->cbegin() == cset->cend());
  CHECK(cset->begin() == cset->end());
  CHECK(cset->cbegin() == cset->end());
  CHECK(cset->begin() == cset->cend());
  CHECK(set.begin() == cset->cbegin());
  CHECK(set.end() == cset->cend());
}

TEST(GraveyardSet, IteratorOneElement) {
  yobiduck::GraveyardSet<uint64_t> set;
  CHECK(set.insert(100ul));
  CHECK(set.contains(100ul));
  yobiduck::GraveyardSet<uint64_t>::iterator it;
  it = set.begin();
  CHECK(it != set.end());
  ++it;
  // CHECK(it == set.end());
  EXPECT_THAT(set, UnorderedElementsAre(100ul));
}

TEST(GraveyardSet, RandomInserts) {
  return;
  absl::BitGen bitgen;
  const size_t j  = 1000;
  yobiduck::GraveyardSet<uint64_t> set;
  absl::flat_hash_set<uint64_t> fset;
  for (size_t i = 0; i < j; ++i) {
    {
      size_t v = absl::Uniform<uint64_t>(bitgen);
      set.insert(v);
      fset.insert(v);
    }
    // We don't yet have the power to check that set is a subset of fset.
    if (i < 10 || i % 60 == 0) {
      EXPECT_THAT(set, UnorderedElementsAreArray(fset));
    }
  }
  EXPECT_THAT(set, UnorderedElementsAreArray(fset));
}

TEST(GraveyardSet, Reserve) {
  {
    yobiduck::GraveyardSet<uint64_t> set;
    set.reserve(1000);
    set.insert(100u);
  }
  {
    yobiduck::GraveyardSet<uint64_t> set;
    set.reserve(1000);
    set.insert(100u);
  }

}

TEST(GraveyardSet, AssignAndReserve) {
  yobiduck::GraveyardSet<uint64_t> set;
  for (size_t i = 1000; i < 2000; ++i) {
    set = yobiduck::GraveyardSet<uint64_t>();
    set.reserve(1000);
  }
}
