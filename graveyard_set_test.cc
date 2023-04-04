#include "graveyard_set.h"

#include <time.h> // for timespec, clock_gettime

#include <cstddef>
#include <cstdint>
#include <functional> // for equal_to
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::Pair;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

using yobiduck::GraveyardSet;

TEST(GraveyardSet, Types) {
  using IntSet = GraveyardSet<uint64_t>;
  static_assert(std::is_same_v<IntSet::key_type, uint64_t>);
  static_assert(std::is_same_v<IntSet::value_type, uint64_t>);
  static_assert(std::is_same_v<IntSet::size_type, size_t>);
  static_assert(std::is_same_v<IntSet::difference_type, ptrdiff_t>);
  static_assert(std::is_same_v<IntSet::hasher, absl::Hash<uint64_t>>);
  static_assert(std::is_same_v<IntSet::key_equal, std::equal_to<uint64_t>>);
  static_assert(
      std::is_same_v<IntSet::allocator_type, std::allocator<uint64_t>>);
  static_assert(std::is_same_v<IntSet::reference, uint64_t &>);
  static_assert(std::is_same_v<IntSet::const_reference, const uint64_t &>);
  static_assert(std::is_same_v<IntSet::pointer, uint64_t *>);
  static_assert(std::is_same_v<IntSet::const_pointer, const uint64_t *>);
}

TEST(GraveyardSet, Basic) {
  GraveyardSet<uint64_t> set;
  EXPECT_EQ(set.size(), 0ul);
  EXPECT_TRUE(!set.contains(0l));
  EXPECT_THAT(set.insert(0ul), Pair(_, true));
  EXPECT_TRUE(set.contains(0l));
  EXPECT_TRUE(!set.contains(1l));
}

TEST(GraveyardSet, EmptyIterator) {
  GraveyardSet<uint64_t> set;
  EXPECT_TRUE(set.begin() == set.end());
}

TEST(GraveyardSet, EmptyConstIterator) {
  GraveyardSet<uint64_t> set;
  EXPECT_TRUE(set.cbegin() == set.cend());
  const GraveyardSet<uint64_t> *cset = &set;
  EXPECT_TRUE(cset->cbegin() == cset->cend());
  EXPECT_TRUE(cset->begin() == cset->end());
  EXPECT_TRUE(cset->cbegin() == cset->end());
  EXPECT_TRUE(cset->begin() == cset->cend());
  EXPECT_TRUE(set.begin() == cset->cbegin());
  EXPECT_TRUE(set.end() == cset->cend());
}

TEST(GraveyardSet, IteratorOneElement) {
  GraveyardSet<uint64_t> set;
  EXPECT_THAT(set.insert(100ul), Pair(_, true));
  EXPECT_TRUE(set.contains(100ul));
  GraveyardSet<uint64_t>::iterator it;
  it = set.begin();
  EXPECT_TRUE(it != set.end());
  ++it;
  CHECK(it == set.end());
  EXPECT_THAT(set, UnorderedElementsAre(100ul));
}

TEST(GraveyardSet, RandomInserts) {
  absl::BitGen bitgen;
  const size_t j = 1000;
  GraveyardSet<uint64_t> set;
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

TEST(GraveyardSet, Rehash0) {
  GraveyardSet<uint64_t> set;
  constexpr size_t N = 1000;
  set.reserve(N);
  for (size_t i = 0; i < N / 2; ++i) {
    set.insert(i);
  }
  set.rehash(0);
}

// TODO: A test that does a rehash after some erases.

namespace {
template <class KeyType> size_t ExpectedCapacityAfterRehash(size_t size) {
  using Traits =
      yobiduck::internal::HashTableTraits<KeyType, void, absl::Hash<KeyType>,
                                          std::equal_to<>,
                                          std::allocator<KeyType>>;
  size_t slots_needed =
      yobiduck::internal::ceil(size * Traits::full_utilization_denominator,
                               Traits::full_utilization_numerator);
  size_t logical_buckets_needed =
      yobiduck::internal::ceil(slots_needed, Traits::kSlotsPerBucket);
  // We have a minor DRY problem here: This magic formula also appears
  // in hash_table.h.
  size_t extra_buckets = (logical_buckets_needed > 4) ? 4
                         : (logical_buckets_needed <= 2)
                             ? 1
                             : logical_buckets_needed - 1;
  size_t physical_buckets = logical_buckets_needed + extra_buckets;
  return physical_buckets * Traits::kSlotsPerBucket;
}
} // namespace

TEST(GraveyardSet, Reserve) {
  GraveyardSet<uint64_t> set;
  set.reserve(1000);
  set.insert(100u);
  EXPECT_EQ(set.size(), 1);
  // There's a tradeoff between making the test easy to read and avoiding the
  // use of inscrutable magic numbers.  So we'll do it both ways.
  EXPECT_EQ(ExpectedCapacityAfterRehash<uint64_t>(1000), 86 * 14);
  EXPECT_EQ(set.capacity(), ExpectedCapacityAfterRehash<uint64_t>(1000));
}

TEST(GraveyardSet, AssignAndReserve) {
  GraveyardSet<uint64_t> set;
  for (size_t i = 1000; i < 2000; ++i) {
    set = GraveyardSet<uint64_t>();
    set.reserve(1000);
  }
}

struct UnhashableInt {
  int x;
  explicit UnhashableInt(int x_value) : x(x_value) {}
};
struct UnhashableIntHasher {
  size_t operator()(const UnhashableInt &h) const {
    return absl::Hash<int>()(h.x);
  }
};
struct UnhashableIntEqual {
  size_t operator()(const UnhashableInt &a, const UnhashableInt &b) const {
    return a.x == b.x;
  }
};

std::ostream &operator<<(std::ostream &stream,
                         const UnhashableInt &unhashable_int) {
  return stream << unhashable_int.x;
}

TEST(GraveyardSet, UserDefinedHashAndEq) {
  {
    absl::flat_hash_set<UnhashableInt, UnhashableIntHasher, UnhashableIntEqual>
        set;
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
    GraveyardSet<UnhashableInt, UnhashableIntHasher,
                           UnhashableIntEqual>
        set;
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
} // namespace

TEST(GraveyardSet, RehashTime) {
  constexpr size_t kSize = 1000000;
  GraveyardSet<size_t> set;
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
    std::cout << "Rehash took " << elapsed / 1'000'000 << "." << std::setw(6)
              << elapsed % 1'000'000 << "ms" << std::endl;
  }
}

ptrdiff_t count_existing = 0;

class NoDefaultConstructor {
public:
  explicit NoDefaultConstructor(int a) : value_(std::make_unique<int>(a)) {
    ++count_existing;
  }
  // No default
  NoDefaultConstructor() = delete;
  // Copy constructor
  NoDefaultConstructor(const NoDefaultConstructor &a)
      : value_(std::make_unique<int>(*a.value_)) {
    ++count_existing;
  }
  // Move constructor
  NoDefaultConstructor(NoDefaultConstructor &&v) : value_(std::move(v.value_)) {
    ++count_existing;
  }
  ~NoDefaultConstructor() { --count_existing; }

private:
  template <typename H>
  friend H AbslHashValue(H h, const NoDefaultConstructor &v) {
    return H::combine(std::move(h), *v.value_);
  }
  friend bool operator==(const NoDefaultConstructor &a,
                         const NoDefaultConstructor &b) {
    return *a.value_ == *b.value_;
  }
  friend std::ostream &operator<<(std::ostream &stream,
                                  const NoDefaultConstructor &a) {
    return stream << *a.value_;
  }

private:
  std::unique_ptr<int> value_;
};

TEST(GraveyardSet, NoDefaultConstructor) {
  // Does it work the same for `graveyard_set` and `flat_hash_set`?
  {
    NoDefaultConstructor v(3);
    NoDefaultConstructor v2 = v;
    CHECK_EQ(v, v2);
    absl::flat_hash_set<NoDefaultConstructor> aset;
    GraveyardSet<NoDefaultConstructor> gset;
    aset.insert(v);
    gset.insert(v);
    for (int i = 100; i < 200; i++) {
      aset.insert(NoDefaultConstructor(i));
      gset.insert(NoDefaultConstructor(i));
    }
  }
  EXPECT_EQ(count_existing, 0);
}

template <class StringSet> void HeterogeneousStringTest(StringSet &set) {
  auto [inserted_it, inserted] = set.insert(std::string("a"));
  std::string as = "a";
  const char *ap = "a";
  char aa[] = "a";
  std::string_view av(as);
  std::string bs = "b";
  const char *bp = "b";
  char ba[] = "b";
  std::string_view bv(bs);
  EXPECT_TRUE(inserted);
  EXPECT_TRUE(set.contains(as));
  EXPECT_TRUE(set.contains(ap));
  EXPECT_TRUE(set.contains(aa));
  EXPECT_TRUE(set.contains(av));
  EXPECT_FALSE(set.contains(bs));
  EXPECT_FALSE(set.contains(bp));
  EXPECT_FALSE(set.contains(ba));
  EXPECT_FALSE(set.contains(bv));

  EXPECT_TRUE(set.find(as) == inserted_it);
  EXPECT_TRUE(set.find(ap) == inserted_it);
  EXPECT_TRUE(set.find(aa) == inserted_it);
  EXPECT_TRUE(set.find(av) == inserted_it);
  EXPECT_TRUE(set.find(bs) == set.end());
  EXPECT_TRUE(set.find(bp) == set.end());
  EXPECT_TRUE(set.find(ba) == set.end());
  EXPECT_TRUE(set.find(bv) == set.end());

  EXPECT_EQ(set.count(as), 1);
  EXPECT_EQ(set.count(ap), 1);
  EXPECT_EQ(set.count(aa), 1);
  EXPECT_EQ(set.count(av), 1);
  EXPECT_EQ(set.count(bs), 0);
  EXPECT_EQ(set.count(bp), 0);
  EXPECT_EQ(set.count(ba), 0);
  EXPECT_EQ(set.count(bv), 0);

  EXPECT_THAT(set.equal_range(as), Pair(Eq(set.find(as)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(ap), Pair(Eq(set.find(as)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(aa), Pair(Eq(set.find(as)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(av), Pair(Eq(set.find(as)), Eq(set.end())));

  EXPECT_THAT(set.equal_range(bs), Pair(Eq(set.find(bs)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(bp), Pair(Eq(set.find(bs)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(ba), Pair(Eq(set.find(bs)), Eq(set.end())));
  EXPECT_THAT(set.equal_range(bv), Pair(Eq(set.find(bs)), Eq(set.end())));
}

TEST(GraveyardSet, Heterogeneous) {
  // Does it work the same for `graveyard_set` and `flat_hash_set`?
  absl::flat_hash_set<std::string> aset;
  GraveyardSet<std::string> gset;
  HeterogeneousStringTest(aset);
  HeterogeneousStringTest(gset);
}

TEST(GraveyardSet, Empty) {
  GraveyardSet<std::string> gset;
  EXPECT_TRUE(gset.empty());
  gset.insert("a");
  EXPECT_FALSE(gset.empty());
}

class Emplaced {
public:
  Emplaced() = delete;
  Emplaced(int a, int b) : a_(a), b_(b) {}

private:
  friend bool operator==(const Emplaced &left, const Emplaced &right) {
    return left.a_ == right.a_;
  }
  friend bool operator!=(const Emplaced &left, const Emplaced &right) {
    return !(left == right);
  }
  template <typename H> friend H AbslHashValue(H h, const Emplaced &v) {
    return H::combine(std::move(h), v.a_);
  }
  int a_, b_;
};

template <class EmplacedSet> void EmplaceTest(EmplacedSet &set) {
  EXPECT_THAT(set.insert(Emplaced(1, 2)), Pair(_, true));
  EXPECT_THAT(set.emplace(3, 4), Pair(_, true));
  EXPECT_TRUE(set.contains(Emplaced(3, 5)));
  EXPECT_EQ(*set.find(Emplaced(3, 6)), Emplaced(3, 7));
  EXPECT_THAT(set.emplace(1, 6), Pair(Eq(set.find(Emplaced(1, 7))), false));
}

TEST(GraveyardSet, Emplace) {
  EXPECT_EQ(Emplaced(3, 4), Emplaced(3, 5));
  EXPECT_NE(Emplaced(3, 3), Emplaced(4, 3));
  absl::flat_hash_set<Emplaced> aset;
  GraveyardSet<Emplaced> gset;
  EmplaceTest(aset);
  EmplaceTest(gset);
}

// A class that needs to be emplaced and supports heterogeneous
// lookup.
class EmplacedHet {
public:
  EmplacedHet() = delete;
  EmplacedHet(int a, int b) : a_(a), b_(b) {}

private:
  friend struct EqEmplacedHet;
  friend struct HashEmplacedHet;
  friend bool operator==(const EmplacedHet &left, const EmplacedHet &right) {
    return left.a_ == right.a_;
  }
  friend bool operator!=(const EmplacedHet &left, const EmplacedHet &right) {
    return !(left == right);
  }
  int a_, b_;
};
struct HashEmplacedHet {
  using is_transparent = void;
  size_t operator()(const EmplacedHet &v) const {
    return absl::Hash<int>()(v.a_);
  }
  size_t operator()(int v) const { return absl::Hash<int>()(v); }
};
struct EqEmplacedHet {
  using is_transparent = void;
  bool operator()(const EmplacedHet &left, const EmplacedHet &right) const {
    return left == right;
  }
  bool operator()(const EmplacedHet &left, int right) const {
    return left.a_ == right;
  }
};

template <class EmplacedSet> void EmplaceHetTest(EmplacedSet &set) {
  EXPECT_THAT(set.insert(EmplacedHet(1, 2)), Pair(_, true));
  EXPECT_THAT(set.emplace(3, 4), Pair(_, true));
  EXPECT_TRUE(set.contains(EmplacedHet(3, 5)));
  EXPECT_EQ(*set.find(EmplacedHet(3, 6)), EmplacedHet(3, 7));
  EXPECT_THAT(set.emplace(1, 6), Pair(Eq(set.find(EmplacedHet(1, 7))), false));
  EXPECT_TRUE(set.contains(3));
}

TEST(GraveyardSet, EmplaceHeterogeneous) {
  EXPECT_EQ(EmplacedHet(3, 4), EmplacedHet(3, 5));
  EXPECT_NE(EmplacedHet(3, 3), EmplacedHet(4, 3));
  EXPECT_TRUE(EqEmplacedHet()(EmplacedHet(3, 5), 3));
  EXPECT_EQ(HashEmplacedHet()(EmplacedHet(3, 4)), HashEmplacedHet()(3));
  absl::flat_hash_set<EmplacedHet, HashEmplacedHet, EqEmplacedHet> aset;
  GraveyardSet<EmplacedHet, HashEmplacedHet, EqEmplacedHet> gset;
  EmplaceHetTest(aset);
  EmplaceHetTest(gset);
}

TEST(GraveyardSet, ErasesIterator1) {
  GraveyardSet<std::string> gset;
  gset.insert("a");
  const auto &cset = gset;
  auto it = cset.find("a");
  EXPECT_EQ(gset.find("a"), it);
  gset.erase(it++);
  EXPECT_EQ(it, gset.end());
  EXPECT_EQ(gset.size(), 0);
  EXPECT_EQ(gset.begin(), gset.end());
}

TEST(GraveyardSet, ErasesIterator100) {
  GraveyardSet<int> gset;
  constexpr size_t kN = 100;
  for (size_t i = 0; i < kN; ++i) {
    gset.insert(i);
  }
  for (size_t i = 0; i < kN; ++i) {
    auto it = gset.find(i);
    gset.erase(it++);
    EXPECT_EQ(gset.size(), kN - i - 1);
    if (it != gset.end()) {
      EXPECT_GT(*it, i);
    }
    for (size_t j = 0; j < kN; ++j) {
      if (j <= i) {
        EXPECT_FALSE(gset.contains(j));
      } else {
        EXPECT_TRUE(gset.contains(j));
      }
    }
  }
  EXPECT_EQ(gset.begin(), gset.end());
}

TEST(GraveyardSet, ErasesIteratorRange) {
  GraveyardSet<int> gset;
  GraveyardSet<int> to_delete;
  constexpr size_t kN = 100;
  for (size_t i = 0; i < kN; ++i) {
    gset.insert(i);
  }
  auto start = gset.cbegin();
  for (size_t i = 0; i < kN / 3; ++i) {
    ++start;
  }
  auto end = start;
  for (size_t i = 0; i < 2 * kN / 3; ++i) {
    to_delete.insert(*end);
    ++end;
  }
  auto end2 = gset.erase(start, end);
  EXPECT_TRUE(end == end2);
  EXPECT_EQ(gset.size() + to_delete.size(), kN);
  for (size_t i = 0; i < kN; ++i) {
    EXPECT_EQ(gset.contains(i), !to_delete.contains(i));
  }
}

TEST(GraveyardSet, ErasesByValue) {
  GraveyardSet<std::string> gset;
  gset.insert("a");
  gset.insert("b");
  gset.insert("c");
  gset.insert("d");
  EXPECT_EQ(gset.erase("a"), 1);
  EXPECT_EQ(gset.erase("a"), 0);
  EXPECT_EQ(gset.erase(std::string("b")), 1);
  EXPECT_EQ(gset.erase(std::string("b")), 0);
  EXPECT_EQ(gset.erase(std::string_view("c")), 1);
  EXPECT_EQ(gset.erase(std::string_view("c")), 0);
  char d[] = "d";
  EXPECT_EQ(gset.erase(d), 1);
  EXPECT_EQ(gset.erase(d), 0);
  EXPECT_TRUE(gset.empty());
}
