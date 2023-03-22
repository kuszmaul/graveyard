#include "graveyard_set.h"

#include <time.h> // for timespec, clock_gettime

#include <cstddef>
#include <cstdint>
#include <functional> // for equal_to
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "absl/log/check.h"
#include "absl/random/random.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Pair;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

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
  static_assert(std::is_same_v<IntSet::reference, uint64_t &>);
  static_assert(std::is_same_v<IntSet::const_reference, const uint64_t &>);
  static_assert(std::is_same_v<IntSet::pointer, uint64_t *>);
  static_assert(std::is_same_v<IntSet::const_pointer, const uint64_t *>);
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
  const yobiduck::GraveyardSet<uint64_t> *cset = &set;
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
  CHECK(it == set.end());
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

TEST(GraveyardSet, Rehash0) {
  yobiduck::GraveyardSet<uint64_t> set;
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
  yobiduck::GraveyardSet<uint64_t> set;
  set.reserve(1000);
  set.insert(100u);
  EXPECT_EQ(set.size(), 1);
  // There's a tradeoff between making the test easy to read and avoiding the
  // use of inscrutable magic numbers.  So we'll do it both ways.
  EXPECT_EQ(ExpectedCapacityAfterRehash<uint64_t>(1000), 86 * 14);
  EXPECT_EQ(set.capacity(), ExpectedCapacityAfterRehash<uint64_t>(1000));
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
    yobiduck::GraveyardSet<UnhashableInt, UnhashableIntHasher,
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
  {
    NoDefaultConstructor v(3);
    NoDefaultConstructor v2 = v;
    CHECK_EQ(v, v2);
    absl::flat_hash_set<NoDefaultConstructor> aset;
    yobiduck::GraveyardSet<NoDefaultConstructor> gset;
    aset.insert(v);
    gset.insert(v);
    for (int i = 100; i < 200; i++) {
      aset.insert(NoDefaultConstructor(i));
      gset.insert(NoDefaultConstructor(i));
    }
  }
  CHECK_EQ(count_existing, 0);
}
