#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tombstone_set.h"

using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

TEST(TombstoneSet, Basic) {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK_EQ(set.size(), 0ul);
  CHECK(!set.contains(0l));
  CHECK(set.insert(0ul));
  CHECK(set.contains(0l));
  CHECK(!set.contains(1l));
}

TEST(TombstoneSet, EmptyIterator) {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK(set.begin() == set.end());
}

TEST(TombstoneSet, EmptyConstIterator) {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK(set.cbegin() == set.cend());
  const yobiduck::TombstoneSet<uint64_t>* cset = &set;
  CHECK(cset->cbegin() == cset->cend());
  CHECK(cset->begin() == cset->end());
  CHECK(cset->cbegin() == cset->end());
  CHECK(cset->begin() == cset->cend());
  CHECK(set.begin() == cset->cbegin());
  CHECK(set.end() == cset->cend());
}

TEST(TombstoneSet, IteratorOneElement) {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK(set.insert(100ul));
  CHECK(set.contains(100ul));
  yobiduck::TombstoneSet<uint64_t>::iterator it;
  it = set.begin();
  CHECK(it != set.end());
  ++it;
  // CHECK(it == set.end());
  EXPECT_THAT(set, UnorderedElementsAre(100ul));
}

TEST(TombstoneSet, RandomInserts) {
  absl::BitGen bitgen;
  yobiduck::TombstoneSet<uint64_t> set;
  absl::flat_hash_set<uint64_t> fset;
  for (size_t i = 0; i < 100; ++i) {
    {
      size_t v = absl::Uniform<uint64_t>(bitgen);
      set.insert(v);
      fset.insert(v);
    }
    // We don't yet have the power to check that set is a subset of fset.
    EXPECT_THAT(set, UnorderedElementsAreArray(fset));
  }
}
