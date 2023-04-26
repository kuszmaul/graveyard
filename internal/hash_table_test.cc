#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <ostream>
#include <unordered_map>
#include <type_traits>

struct Counts {
  size_t copy_constructions = 0;
  size_t move_constructions = 0;
  size_t copy_assignments = 0;
  size_t move_assignments = 0;
  size_t destructions = 0;
  Counts() = default;
  Counts(size_t cc, size_t mc, size_t ca, size_t ma, size_t d) :copy_constructions(cc), move_constructions(mc), copy_assignments(ca), move_assignments(ma), destructions(d) {}
  friend std::ostream& operator<<(std::ostream& os, const Counts& counts) {
    return os << "cc=" << counts.copy_constructions << " mc=" << counts.move_constructions << " ca=" << counts.copy_assignments << " ma=" << counts.move_assignments << " d=" << counts.destructions;
  }
  friend bool operator==(const Counts &a, const Counts &b) {
    return
        a.copy_constructions == b.copy_constructions &&
        a.move_constructions == b.move_constructions &&
        a.copy_assignments == b.copy_assignments &&
        a.move_assignments == b.move_assignments &&
        a.destructions == b.destructions;
  }
} counts;

class StandardLayout {
 public:
  StandardLayout() = default;
  explicit StandardLayout(int n) :x_(std::make_unique<int>(n)) {}
  // Copy constructor
  StandardLayout(const StandardLayout& l) :x_(std::make_unique<int>(l.value())) {
    ++counts.copy_constructions;
  }
  // Move constructor
  StandardLayout(StandardLayout&& l) :x_(std::move(l.x_)) {
    l.x_ = std::make_unique<int>(0);
    ++counts.move_constructions;
  }
  ~StandardLayout() {
    ++counts.destructions;
  }
  // Copy assignment operator
  StandardLayout& operator=(const StandardLayout& other) {
    x_ = std::make_unique<int>(other.value());
    return *this;
  }
  // Move assignment operator
  StandardLayout& operator=(StandardLayout&& other) {
    x_ = std::move(other.x_);
    other.x_ = std::make_unique<int>(0);
    return *this;
  }
  //StandardLayout(StandardLayout&&) = default;
  int value() const { return *x_; }
 private:
  template <typename H>
  friend H AbslHashValue(H h, const StandardLayout &v) {
    return H::combine(std::move(h), v.value());
  }
  friend bool operator==(const StandardLayout &a, const StandardLayout &b) {
    return a.value() == b.value();
  }

  std::unique_ptr<int> x_ = std::make_unique<int>(0);

};

class NonStandardLayout : public StandardLayout {
 public:
  using StandardLayout::StandardLayout;
  virtual void Foo() {
    return;
  }
};

static_assert(std::is_standard_layout_v<StandardLayout>);
static_assert(!std::is_standard_layout_v<NonStandardLayout>);

TEST(Layout, StandardLayoutTest) {
  StandardLayout y{3};
  EXPECT_EQ(y.value(), 3);
  StandardLayout z = std::move(y);
  EXPECT_EQ(y.value(), 0);
  EXPECT_EQ(z.value(), 3);
  StandardLayout w{std::move(z)};
  EXPECT_EQ(w.value(), 3);
  StandardLayout z2;
  z2 = std::move(w);
  EXPECT_EQ(z2.value(), 3);
}

// If has-nodes is true, then we expect that moving a table will
// result in no constructions or destructions of elements.
template<class KeyType, template<typename, typename, typename> class TableType>
void TestLayout(bool is_standard_layout, bool has_nodes) {
  counts = Counts();
  {
    TableType<KeyType, std::string, absl::Hash<KeyType>> s;
    s.reserve(10);
    s.emplace(KeyType(2), "hello2");
    EXPECT_EQ(counts, Counts(0, 1, 0, 0, 1));
    counts = Counts();
    // For this insert we expect to see a 2 total copy or move
    // constructions and two destructions.
    s.insert({KeyType(3), "hello"});
    EXPECT_EQ(counts.copy_constructions + counts.move_constructions, 2);
    EXPECT_EQ(counts.destructions, 2);
    counts = Counts();
    s[KeyType{4}] = "goodbye";
    EXPECT_EQ(counts, Counts(0, 1, 0, 0, 1));
    auto s2 = std::move(s);
    EXPECT_EQ(counts, Counts(0, 1, 0, 0, 1));
    counts = Counts();
    s2.rehash(0);
    if (has_nodes) {
      // For containers such as unordered_map, which have nodes.
      EXPECT_EQ(counts, Counts(0, 0, 0, 0, 0));
    } else if (is_standard_layout) {
      // For containers such abseil, we have to do moves for standard layouts
      EXPECT_EQ(counts, Counts(0, 3, 0, 0, 3));
    } else {
      // For containers such abseil, we have to do copies for nonstandard layouts
      EXPECT_EQ(counts, Counts(3, 0, 0, 0, 3));
    }
    counts = Counts();
  }
  EXPECT_EQ(counts, Counts(0, 0, 0, 0 , 3));
}

TEST(Layout, StandardLayoutInUnorderedMap) {
  TestLayout<StandardLayout, std::unordered_map>(true, true);
}
TEST(Layout, NonStandardLayoutInUnorderedMap) {
  TestLayout<NonStandardLayout, std::unordered_map>(false, true);
}
TEST(Layout, StandardLayoutInAbsl) {
  TestLayout<StandardLayout, absl::flat_hash_map>(true, false);
}
TEST(Layout, NonStandardLayoutInAbsl) {
  TestLayout<NonStandardLayout, absl::flat_hash_map>(false, false);
}
