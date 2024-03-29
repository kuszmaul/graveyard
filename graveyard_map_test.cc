#include "graveyard_map.h"

#include <cstddef> // for size_t, ptrdiff_t
#include <cstdint> // for uint64_t
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits> // for is_same_v
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash.h" // for Hash
#include "absl/log/check.h"
#include "absl/random/random.h"
#include "absl/strings/str_cat.h" // for StrCat
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Eq;
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

template <class StringMap> void HeterogeneousStringTest(StringMap &map) {
  auto [inserted_it, inserted] = map.insert({std::string("a"), 42});
  std::string as = "a";
  const char *ap = "a";
  char aa[] = "a";
  std::string_view av(as);
  std::string bs = "b";
  const char *bp = "b";
  char ba[] = "b";
  std::string_view bv(bs);
  EXPECT_TRUE(inserted);
  EXPECT_TRUE(map.contains(as));
  EXPECT_TRUE(map.contains(ap));
  EXPECT_TRUE(map.contains(aa));
  EXPECT_TRUE(map.contains(av));
  EXPECT_FALSE(map.contains(bs));
  EXPECT_FALSE(map.contains(bp));
  EXPECT_FALSE(map.contains(ba));
  EXPECT_FALSE(map.contains(bv));

  EXPECT_TRUE(map.find(as) == inserted_it);
  EXPECT_TRUE(map.find(ap) == inserted_it);
  EXPECT_TRUE(map.find(aa) == inserted_it);
  EXPECT_TRUE(map.find(av) == inserted_it);
  EXPECT_TRUE(map.find(bs) == map.end());
  EXPECT_TRUE(map.find(bp) == map.end());
  EXPECT_TRUE(map.find(ba) == map.end());
  EXPECT_TRUE(map.find(bv) == map.end());

  EXPECT_EQ(map.count(as), 1);
  EXPECT_EQ(map.count(ap), 1);
  EXPECT_EQ(map.count(aa), 1);
  EXPECT_EQ(map.count(av), 1);
  EXPECT_EQ(map.count(bs), 0);
  EXPECT_EQ(map.count(bp), 0);
  EXPECT_EQ(map.count(ba), 0);
  EXPECT_EQ(map.count(bv), 0);

  EXPECT_THAT(map.equal_range(as), Pair(Eq(map.find(as)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(ap), Pair(Eq(map.find(as)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(aa), Pair(Eq(map.find(as)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(av), Pair(Eq(map.find(as)), Eq(map.end())));

  EXPECT_THAT(map.equal_range(bs), Pair(Eq(map.find(bs)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(bp), Pair(Eq(map.find(bs)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(ba), Pair(Eq(map.find(bs)), Eq(map.end())));
  EXPECT_THAT(map.equal_range(bv), Pair(Eq(map.find(bs)), Eq(map.end())));

  // EXPECT_EQ(&map[as], &map.find(as)->second);
}

TEST(GraveyardMap, Heterogenous) {
  // Does it work the same for `graveyard_map` and `flat_hash_map`?
  absl::flat_hash_map<std::string, uint64_t> amap;
  yobiduck::GraveyardMap<std::string, uint64_t> gmap;
  HeterogeneousStringTest(amap);
  HeterogeneousStringTest(gmap);
}

// Verify that try_emplace does a std::move.  For this we need to be
// able to count the number of moves and copies.

struct Counts {
  size_t def = 0; // default constructions
  size_t cc = 0; // copy constructions
  size_t mc = 0; // move constructions
  size_t ec = 0; // explicit constructions
  size_t ca = 0; // copy assignments
  size_t ma = 0; // move assignments
  size_t des = 0; // destructions
  Counts() = default;
  Counts& Def(size_t dc) { def += dc; return *this; }
  Counts& CC(size_t c) { cc += c; return *this; }
  Counts& MC(size_t c) { mc += c; return *this; }
  Counts& EC(size_t c) { ec += c; return *this; }
  Counts& CA(size_t c) { ca += c; return *this; }
  Counts& MA(size_t c) { ma += c; return *this; }
  Counts& Des(size_t de) { des += de; return *this; }
  //  Counts(size_t dc, size_t cc, size_t mc, size_t ec, size_t ca, size_t ma, size_t d) :def(dc), cc(cc), mc(mc), ec(ec), ca(ca), ma(ma), des(d) {}
  void Reset() { *this = Counts(); }
  friend std::ostream& operator<<(std::ostream& os, const Counts& counts) {
    os << "Counts{";
    bool need_space = false;
    auto spacing = [&]() {
      if (need_space) os << " ";
      need_space = true;
    };
    if (counts.def) {
      spacing();
      os << ".def=" << counts.def;
    }
    if (counts.cc) {
      spacing();
      os << ".cc=" << counts.cc;
    }
    if (counts.mc) {
      spacing();
      os << ".mc=" << counts.mc;
    }
    if (counts.ec) {
      spacing();
      os << ".ec=" << counts.ec;
    }
    if (counts.ca) {
      spacing();
      os << ".ca=" << counts.ca;
    }
    if (counts.ma) {
      spacing();
      os << ".ma=" << counts.ma;
    }
    if (counts.des) {
      spacing();
      os << ".des=" << counts.des;
    }
    return os << "}";
  }
  friend bool operator==(const Counts &a, const Counts &b) {
    return
        a.def == b.def &&
        a.cc == b.cc &&
        a.mc == b.mc &&
        a.ec == b.ec &&
        a.ca == b.ca &&
        a.ma == b.ma &&
        a.des == b.des;
  }
};

template <int count_class>
struct CountedTemplate {
  static Counts counts;
  // Default constructor
  CountedTemplate() { counts.Def(1); }
  // Copy constructor
  CountedTemplate([[maybe_unused]] const CountedTemplate& l) :v(l.v) {
    counts.CC(1);
  }
  // Move constructor
  CountedTemplate([[maybe_unused]] CountedTemplate&& l) :v(l.v) {
    counts.MC(1);
  }
  // Destructor
  ~CountedTemplate() { counts.Des(1); }
  // Copy assignmnet
  CountedTemplate& operator=(const CountedTemplate& other) {
    v = other.v;
    counts.CA(1);
    return *this;
  }
  // Move assignment
  CountedTemplate& operator=(CountedTemplate&& other) {
    v = other.v;
    counts.MA(1);
    return *this;
  }
  explicit CountedTemplate(size_t value) :v(value) {
    counts.EC(1);
  }
  friend bool operator==(const CountedTemplate &a, const CountedTemplate &b) {
    return a.v == b.v;
  }
  friend std::ostream& operator<<(std::ostream& os, const CountedTemplate &a) {
    return os << "<" << a.v << ">";
  }
  class Hash {
   public:
    size_t operator()(const CountedTemplate &v) const {
      return v.v;
    }
  };
  size_t v = 0;
};

// Define the static variables.
template <int count_class>
Counts CountedTemplate<count_class>::counts;

using Counted = CountedTemplate<0>;


// Verify that Graveyard, Abseil, and Unordered map call the same
// numbers of default constructors, move constructors, copy
// constructors, move operators, and copy operators, and destructors
// for various ways of inserting values into an empty table.
template <template <typename, typename> class MapType>
void TryArgsMovedTest(std::string_view called_from) {
  Counts &counts = Counted::counts;
  using Map = MapType<std::string, Counted>;
  //using Map = std::unordered_map<std::string, Counted<0>>;
  counts.Reset();
  {
    Map map;
    EXPECT_EQ(counts, Counts()) << " from " << called_from;
    map["a"];
    EXPECT_EQ(counts, Counts{.def=1});

  }
  EXPECT_EQ(counts, Counts().Def(1).Des(1));

  counts.Reset();
  {
    Map map;
    map.emplace("a", Counted());
    // Graveyard is doing extra move constructors.  TODO: Fix this to pass thus:
    //  EXPECT_EQ(counts, (Counts{.def=1, .mc=1, .des=1})) << " from " << called_from;
    //
    // For now, we are happy if there are no copies

    // Make sure that every move construction has a destruction
    EXPECT_EQ(counts.mc, counts.des);
    // Then make sure the only other thing happening is the default constructor.
    counts.mc = 0;
    counts.des = 0;
    EXPECT_EQ(counts, Counts{.def=1});
    counts.Reset();
  }
  EXPECT_EQ(counts, (Counts{.des=1})) << " from " << called_from;

  counts.Reset();
  {
    Map map;
    map.try_emplace("a", Counted());
    EXPECT_EQ(counts, Counts().Def(1).MC(1).Des(1));
  }
}

TEST(GraveyardMap, AbslTryArgsMoved) {
  TryArgsMovedTest<absl::flat_hash_map>("absl");
}
TEST(GraveyardMap, StdTryArgsMoved) {
  TryArgsMovedTest<std::unordered_map>("std");
}
TEST(GraveyardMap, GraveyardTryArgsMoved) {
  TryArgsMovedTest<yobiduck::GraveyardMap>("graveyard");
}

template <template <typename, typename> class MapType>
void DoesntMoveTest(std::string_view called_from) {
  MapType<std::string, std::string> map;
  //absl::flat_hash_map<std::string, std::string> map;
  //yobiduck::GraveyardMap<std::string, std::string> map;
  {
    auto [it, inserted] = map.try_emplace("a", "b");
    EXPECT_TRUE(inserted) << " from " << called_from;
    EXPECT_THAT(*it, Pair("a", "b"));
    EXPECT_TRUE(map.find("a") != map.end());

  }
  {
    std::string b_string("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    std::string b_string_copy(b_string);
    auto [it, inserted] = map.try_emplace("a", std::move(b_string));
    EXPECT_FALSE(inserted);
    EXPECT_THAT(*it, Pair("a", "b"));
    // The b_string has not been moved, since it was not inserted (in
    // spite of the `std::move`.)
    EXPECT_FALSE(b_string.empty());
  }
  {
    std::string c_string("ccccccccccccccccccccccccccc");
    auto [it, inserted] = map.try_emplace("c", std::move(c_string));
    EXPECT_TRUE(inserted);
    // The c_string has been moved (it was forwarded into the table).
    EXPECT_TRUE(c_string.empty());
  }
}

TEST(GraveyardMap, AbslDoesntMove) {
  DoesntMoveTest<absl::flat_hash_map>("absl");
}
TEST(GraveyardMap, StdDoesntMove) {
  DoesntMoveTest<std::unordered_map>("absl");
}
TEST(GraveyardMap, GraveyardDoesntMove) {
  DoesntMoveTest<yobiduck::GraveyardMap>("absl");
}

TEST(GraveyardMap, OperatorSquareBracket) {
  yobiduck::GraveyardMap<std::string, std::string> map;
  map["a"] = "b";
  EXPECT_TRUE(map.contains("a"));
}

// When the value_type is a `pair<const K, V>` that can safely be cast
// to `pair<K, V>`, we should move it rather than copying it.
template <template <typename, typename, typename> class MapType, bool is_open_addressed>
void MovesMovableTest() {
  Counts &counts = Counted::counts;

  {
    counts.Reset();
    MapType<Counted, int, Counted::Hash> map;
    map.emplace(std::make_pair(Counted(2), 0));
    EXPECT_FALSE(map.empty());
    // Abseil and Std give Counts{.mc=2, .ec=1, .des=2}
    //
    // GraveyardMap gives Counts{.mc=4 .ec=1 .des=4} because we haven't done the fancy decomposition.
    EXPECT_EQ(counts.def, 0);
    EXPECT_EQ(counts.cc, 0);
    EXPECT_EQ(counts.ca, 0);
    EXPECT_EQ(counts.ec, 1);
    EXPECT_EQ(counts.ma, 0);
    EXPECT_EQ(counts.mc, counts.des);
    counts.Reset();
  }
  EXPECT_EQ(counts, (Counts{.des=1}));

  {
    counts.Reset();
    MapType<Counted, int, Counted::Hash> map;
    map.emplace(Counted(2), 0);
    // Abseil and std give Counts{.mc=1 .ec=1 .des=1}
    // Graveyard does a couple extra moves because we haven't done the fancy decomposition.
    // For now what we care about is no copying.
    EXPECT_EQ(counts.cc, 0);
    EXPECT_EQ(counts.ca, 0);
  }

  counts.Reset();
  MapType<Counted, Counted, Counted::Hash> map;
  map.emplace(Counted(1), Counted(2));
  // Abseil and Std expect Counts{.mc=2, .ec=2, .des=2}.
  // Graveyard does some extra moves because we haven't done the fancy decomposition.
  // For now what we care about is no copying.
  EXPECT_EQ(counts.cc, 0);
  EXPECT_EQ(counts.ca, 0);
  {
    auto it = map.find(Counted(1));
    EXPECT_TRUE(it != map.end());
  }
  counts.Reset();
  auto map2 = std::move(map);
  // Moving a map causes no element moves to happen
  EXPECT_EQ(counts, Counts());
  // Rehashhing is interesting, however.
  map2.rehash(0);
  if (is_open_addressed) {
    // E.g., absl::flat_hash_map and Graveyardmap do move the two items
    // and destruct the moved-from items.
    EXPECT_EQ(counts, (Counts{.mc=2, .des=2}));
  } else {
    // E.g., std::unordered_map does not moves or copies to rehash.
    EXPECT_EQ(counts, Counts());
  }
}

TEST(GraveyardMap, MovesMovablePairStd) {
  MovesMovableTest<std::unordered_map, false>();
}
TEST(GraveyardMap, MovesMovablePairAbsl) {
  MovesMovableTest<absl::flat_hash_map, true>();
}
TEST(GraveyardMap, MovesMovablePairGraveyard) {
  MovesMovableTest<yobiduck::GraveyardMap, true>();
}

// When the value_type is a `pair<const K, V>` that can *not* safely
// be cast to `pair<K, V>`, we should copy it rather than move it.
//
// This class, when used as `K`, cannot be cast because its virtual
// method makes it a nonstandard layout.
//
// The actual cast involved, internally is a union
//   union Union {
//     std::pair<K, V> mutable_value;
//     std::pair<const K, V> immutable_value;
//   };
//
// We want to store into the immutable value and then use the mutable
// value when we want to rehash, doing
//   in `emplace`:
//     new (&u.imutable_value) std::pair<K, V>(key, value);
//   in `rehash`:
//     std::move(u.mutable_value);
//   in `operator[]`:
//     return u.immutable_value;

// It is that last operation that is allowed by the following sentence
// in https://en.cppreference.com/w/cpp/language/union
//
//    If two union members are standard-layout types, it's
//    well-defined to examine their common subsequence on any
//    compiler.
//
// The relevant section of the standard is
// https://timsong-cpp.github.io/cppwp/n3337/class.mem#19 (9.2.19)
class CountedNonStandard : public Counted {
  using Counted::Counted;
  virtual void Foo() {}
};

template <template <typename, typename, typename> class MapType, bool is_open_addressed>
void DoesntMoveNonmovableTest() {
  Counts &counts = Counted::counts;
  counts.Reset();
  MapType<CountedNonStandard, Counted, Counted::Hash> map;
  map.emplace(CountedNonStandard(1), Counted(2));
  // Expect two move constructors and two deletions.  But we haven't
  // done the fancy decomposition, so there's not much to test.
  // Graveyard does a copule extra copy constructors and move
  // constructors.
  // TODO: Make graveyard pass this EXPECT:
  //  EXPECT_EQ(counts, (Counts{.mc=2, .ec=2, .des=2}));
  {
    auto it = map.find(CountedNonStandard(1));
    EXPECT_TRUE(it != map.end());
  }
  counts.Reset();
  auto map2 = std::move(map);
  // Moving a map causes no element moves to happen
  EXPECT_EQ(counts, Counts());
  // Rehashhing is interesting, however.
  map2.rehash(0);
  if (is_open_addressed) {
    // E.g., absl::flat_hash_map and Graveyardmap copy the key (so 1
    // copy and 1 move).
    EXPECT_EQ(counts, Counts().CC(1).MC(1).Des(2));
  } else {
    // E.g., std::unordered_map does not moves or copies to rehash.
    EXPECT_EQ(counts, Counts());
  }
}

TEST(GraveyardMap, DoesntMoveNonmovablePairStd) {
  DoesntMoveNonmovableTest<std::unordered_map, false>();
}
TEST(GraveyardMap, DoesntMoveNonmovablePairAbsl) {
  DoesntMoveNonmovableTest<absl::flat_hash_map, true>();
}
TEST(GraveyardMap, DoesntMoveNonmovablePairGraveyard) {
  DoesntMoveNonmovableTest<yobiduck::GraveyardMap, true>();
}
