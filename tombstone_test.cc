#include "tombstone_set.h"

#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using testing::UnorderedElementsAreArray;

TEST(TombstoneSet, Basic) {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK_EQ(set.size(), 0ul);
  CHECK(!set.contains(0l));
  CHECK(set.insert(0ul));
  CHECK(set.contains(0l));
  CHECK(!set.contains(1l));
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
    // Don't have iterators yet so to do this
    //   EXPECT_THAT(set, UnorderedElementsAreArray(fset));
    // Do this
    // 1) Check that fset is a subset of set.
    for (uint64_t v : fset) {
      CHECK(set.contains(v));
    }
    // We don't yet have the power to check that set is a subset of fset.
 }
}
