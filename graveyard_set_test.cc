#include "graveyard_set.h"

#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::Pair;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;
using testing::_;

TEST(GraveyardSet, Types) {
  using IntSet = yobiduck::GraveyardSet<uint64_t>;
  static_assert(std::is_same_v<IntSet::key_type, uint64_t>);
  static_assert(std::is_same_v<IntSet::value_type, uint64_t>);
  static_assert(std::is_same_v<IntSet::size_type, size_t>);
  static_assert(std::is_same_v<IntSet::difference_type, ptrdiff_t>);
  static_assert(std::is_same_v<IntSet::hasher, absl::Hash<uint64_t>>);
  static_assert(std::is_same_v<IntSet::key_equal, std::equal_to<uint64_t>>);
  static_assert(
      std::is_same_v<IntSet::allocator_type, std::allocator<uint64_t>>);
  static_assert(std::is_same_v<IntSet::reference, uint64_t&>);
  static_assert(std::is_same_v<IntSet::const_reference, const uint64_t&>);
  static_assert(std::is_same_v<IntSet::pointer, uint64_t*>);
  static_assert(std::is_same_v<IntSet::const_pointer, const uint64_t*>);
}

TEST(GraveyardSet, Basic) {
  yobiduck::GraveyardSet<uint64_t> set;
  EXPECT_EQ(set.size(), 0ul);
  EXPECT_TRUE(!set.contains(0l));
  EXPECT_THAT(set.insert(0ul), Pair(_, true));
  EXPECT_TRUE(set.contains(0l));
  EXPECT_TRUE(!set.contains(1l));
}

TEST(GraveyardSet, EmptyIterator) {
  yobiduck::GraveyardSet<uint64_t> set;
  EXPECT_TRUE(set.begin() == set.end());
}

TEST(GraveyardSet, EmptyConstIterator) {
  yobiduck::GraveyardSet<uint64_t> set;
  EXPECT_TRUE(set.cbegin() == set.cend());
  const yobiduck::GraveyardSet<uint64_t>* cset = &set;
  EXPECT_TRUE(cset->cbegin() == cset->cend());
  EXPECT_TRUE(cset->begin() == cset->end());
  EXPECT_TRUE(cset->cbegin() == cset->end());
  EXPECT_TRUE(cset->begin() == cset->cend());
  EXPECT_TRUE(set.begin() == cset->cbegin());
  EXPECT_TRUE(set.end() == cset->cend());
}

TEST(GraveyardSet, IteratorOneElement) {
  yobiduck::GraveyardSet<uint64_t> set;
  EXPECT_THAT(set.insert(100ul), Pair(_, true));
  EXPECT_TRUE(set.contains(100ul));
  yobiduck::GraveyardSet<uint64_t>::iterator it;
  it = set.begin();
  EXPECT_TRUE(it != set.end());
  ++it;
  // CHECK(it == set.end());
  EXPECT_THAT(set, UnorderedElementsAre(100ul));
}

TEST(GraveyardSet, RandomInserts) {
  absl::BitGen bitgen;
  const size_t j = 1000;
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

struct UnhashableInt {
  int x;
  explicit UnhashableInt(int x) :x(x) {}
};
struct UnhashableIntHasher {
  size_t operator()(const UnhashableInt &h) const { return absl::Hash<int>()(h.x); }
};
struct UnhashableIntEqual {
  size_t operator()(const UnhashableInt &a, const UnhashableInt &b) const { return a.x == b.x; }
};

std::ostream& operator<<(std::ostream& stream, const UnhashableInt& unhashable_int) {
  return stream << unhashable_int.x;
}

TEST(GraveyardSet, UserDefinedHashAndEq) {
  {
    absl::flat_hash_set<UnhashableInt, UnhashableIntHasher, UnhashableIntEqual> set;
    EXPECT_TRUE(!set.contains(UnhashableInt{0}));
    set.insert(UnhashableInt{0});
    EXPECT_TRUE(set.contains(UnhashableInt{0}));
    for (int i = 0; i < 1000; i++) {
      set.insert(UnhashableInt{i});
    }
    for (int i = 0; i < 1000; i++) {
      EXPECT_TRUE(set.contains(UnhashableInt{i}));
    }
  }
  {
    yobiduck::GraveyardSet<UnhashableInt, UnhashableIntHasher, UnhashableIntEqual> set;
    EXPECT_TRUE(!set.contains(UnhashableInt{0}));
    set.insert(UnhashableInt{0});
    EXPECT_TRUE(set.contains(UnhashableInt{0}));
    for (int i = 0; i < 10000; ++i) {
      set.insert(UnhashableInt{i});
      for (int j = 0; j < i; ++j) {
        EXPECT_TRUE(set.contains(UnhashableInt{j})) << "i=" << i << " j=" << j;
      }
    }
    for (int i = 0; i < 10000; ++i) {
      EXPECT_TRUE(set.contains(UnhashableInt{i})) << "i=" << i;
    }
  }
}

struct IdentityHasher {
  size_t operator()(const size_t v) const { return v; }
};

namespace {
inline uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}
}  // namespace

TEST(GraveyardSet, RehashTime) {
  constexpr size_t kSize = 1000000;
  yobiduck::GraveyardSet<size_t> set;
  set.reserve(kSize);
  for (size_t i = 0; i < kSize; ++i) {
    set.insert(i);
  }
  for (size_t j = 0; j < 3; ++j) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    set.rehash((set.size() * 8 + 6) / 7);
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t elapsed = end - start;
    std::cout << "Rehash took " << elapsed / 1'000'000 << "." << std::setw(6) << elapsed % 1'000'000 << "ms" << std::endl;
  }
}


#if 0
/// This will overflow the counters:  So we should switch to something like std::unordered_map.
struct UnhashableIntIdentityHasher {
  size_t operator()(const UnhashableInt &h) const { return h.x; }
};

TEST(GraveyardSet, SortedBucketIteratorBadHash) {
  yobiduck::GraveyardSet<UnhashableInt, UnhashableIntIdentityHasher, UnhashableIntEqual> graveyard_set;
  std::set<int> std_set;
  absl::BitGen bitgen;
  int kCount = 10000;
  for (int i = 0; i < kCount; ++i) {
    int v = absl::Uniform(bitgen, 0, kCount * kCount);
    graveyard_set.insert(UnhashableInt{v});
    std_set.insert(v);
  }
  auto std_it = std_set.begin();
  for (auto heap_element : graveyard_set.GetSortedBucketsIterator()) {
    EXPECT_TRUE(std_it != std_set.end());
    EXPECT_EQ(heap_element.value->x, *std_it);
    ++std_it;
  }
}
#endif
