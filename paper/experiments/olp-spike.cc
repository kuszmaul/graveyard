/* Measure the spike for a hovering workload with ordered linear
 * probing. */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hash_function.h"

class UniqueNumbers {
 public:
  explicit UniqueNumbers(std::mt19937_64 &gen) :gen_(gen) {}
  uint64_t Next() {
    while (true) {
      uint64_t v = distribution_(gen_);
      auto [it, inserted] = seen_.insert(v);
      if (inserted) {
        return v;
      }
    }
  }
 private:
  std::mt19937_64 &gen_;
  std::uniform_int_distribution<uint64_t> distribution_;
  std::unordered_set<uint64_t> seen_;
};

struct ProbeLengths {
  double found, notfound, insert;
};

// A 64-bit integer with a special printer
struct PV {
  uint64_t v;
  explicit PV(uint64_t v) :v(v) {}
  friend std::ostream& operator<<(std::ostream& os, PV pv) {
    return os << (pv.v >> 54) << "." << std::hex << (pv.v % 16) << std::dec;
  }
};


// This OLP cannot resize.
class Olp {
 public:
  explicit Olp(size_t capacity) :slots_(capacity) {}
  void insert(uint64_t v) {
    // std::cout << __LINE__ << ": Inserting " << PV(v) << " into " << *this << std::endl;
    const size_t capacity = slots_.size();
    assert(size_ < capacity);
    const size_t base_h1 = H1(v, capacity);
    size_t h1 = base_h1;  // h1 of v.
    size_t off_since_v = 0; // offset since we changed v.
    size_t index = h1;
    bool wrapped = false;
    // We may need to wrap around several times if there are tombstones that used up the empty slots.
    for (size_t off = 0; true; ++off, ++off_since_v, ++index) {
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      // std::cout << __LINE__ << ":  Trying to insert " << PV(v) << " at " << index << std::endl;
      Slot& slot = slots_[index];
      const Slot to_insert{wrapped, v};
      switch (slot.tag()) {
        case Tag::kEmpty: {
          ++size_;
          slot = to_insert;
          return;
        }
        case Tag::kPresent: {
          assert(slot.value() != v);
          if (to_insert < slot) {
            // std::cout << __LINE__ << ":  at " << index << " swapping slotval=" << to_insert << " into slotval=" << slot << std::endl;
            v = slot.value();
            h1 = H1(v, capacity);
            off_since_v = 0;
            wrapped = slot.wrapped();
            slot = to_insert;
            continue;
          }
          break;
        }
        case Tag::kTombstone: {
          // std::cout << __LINE__ << ": " << slot << " <? " << to_insert << std::endl;
          if (! (slot < to_insert)) {
            ++size_;
            slot = to_insert;
            return;
          }
          break;
        }
      }
    }
    std::cerr << "Table full" << std::endl;
    abort();
  }
  bool erase(uint64_t v) {
    const size_t capacity = slots_.size();
    assert(size_ < capacity);
    assert(size_ > 0);
    size_t h1 = H1(v, capacity);
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      bool wrapped = false;
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      Slot to_erase{wrapped, v};
      switch (slots_[index].tag()) {
        case Tag::kEmpty: {
          // It's not there.
          return false;
        }
        case Tag::kPresent: {
          if (slots_[index].value() == v) {
            slots_[index].ConvertToTombstone();
            --size_;
            return true;
          }
          if (to_erase < slots_[index]) {
            return false;
          }
          break;
        }
        case Tag::kTombstone: {
          if (to_erase < slots_[index]) {
            return false;
          }
          break;
        }
      }
    }
    return false;
  }

  bool contains(uint64_t v) const {
    return Contains(v).first;
  }

  void Validate() const {
    Slot prev;
    size_t count = 0;
    size_t i = 0;
    for (const auto& slot : slots_) {
      if (!slot.empty() && !prev.empty()) {
        assert(prev < slot);
      }
      if (slot.present()) {
        ++count;
        prev = slot;
        if (!contains(slot.value())) {
          std::cout << "Looking for " << (slot.value() >> 54) << " in " << *this << std::endl;
        }
        assert(contains(slot.value()));
      }
      if (slot.tombstone()) {
        assert(!contains(slot.value()));
      }
      ++i;
    }
    assert(count == size_);
  }

#if 0
  ProbeLengths GetProbeLengths() const {
    return ProbeLengths{
      .found = FoundAverageProbeLength(),
      .notfound = NotFoundAverageProbeLength(),
      .insert = InsertAverageProbeLength()};
  }
#endif

 private:
#if 0
  // Computes the average probe length for a successful lookup by
  // iterating through the present values and determining the probe
  // length to find each value.
  double FoundAverageProbeLength() const {
    double n = 0;
    size_t found_sum = 0;
    for (const auto& slot : slots_) {
      if (slot.present()) {
        auto [found, length] = Contains(slot.value());
        assert(found);
        ++n;
        found_sum += length;
      }
    }
    return found_sum / n;
  }

  // Computes the average probe length for an unsuccessful lookup by
  // iterating over the slots and computing, for each slot, the
  // average cost of an unsuccessful lookup into that slot.
  double NotFoundAverageProbeLength() const {
    double sum = 0;
    for (size_t i = 0; i < slots_.size(); ++i) {
      sum += NotFoundAverageProbeLength(i);
    }
    return sum / slots_.size();
  }

  // Computes the average probe legnth for an unsuccessful lookup in a
  // given slot by iterating through the values and taking the average
  // probe length over the values (including tombstones) that prefer
  // this slot.
  double NotFoundAverageProbeLength(size_t slot_number) const;

  double InsertAverageProbeLength(UniqueNumbers &numbers) const {
    const size_t capacity = slots_.size();
    const size_t kNumberOfTrails = capacity * 16;
    for (size_t i = 0; i < kNumberOfTrials; ++i) {


    for (size_t i = 0; i < capacity; ++i) {
      sum += InsertAverageProbeLength(i);
    }
    return sum / capacity;
  }

  double InsertAverageProbeLength(size_t slot_number) const {
    const size_t capacity = slots_.size();
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      if (index >= capacity) {
        index -= capacity;
      }

  }
#endif

#if 0

  double NotFoundAverageProbeLength(size_t slot_number) const {
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      bool wrap = false
      if (index >= capacity) {
        index -= capacity;
      }
      Slot& slot = slots_[index];
      if (slot.empty()) {
        continue;
      }
      size_t h1 = H1(slot.value(), capacity);


  }

  double NotFoundAverageProbeLength(UniqueNumbers &unique_numbers) const {
    // Computing the average is tricky, since it may end early based
    // on comparing the value to what's present.  So let's just pick
    // some random never-before-seen values and compute their probe
    // lengths.
    const size_t kN = slots_.size() * 10;
    double sum = 0;
    for (size_t i = 0; i < kN; ++i) {
      auto [found, length] = Contains(unique_numbers.Next());
      assert(!found);
      sum += length;
    }
    return sum / kN;
  }
  double InsertAverageProbeLength(UniqueNumbers &unique_numbers) const {
    const size_t kN = slots_.size() * 10;
    double n = 0;
    double sum = 0;
    for (size_t i = 0; i < kN; ++i) {
      sum += InsertProbeLength(unique_numbers.Next());
    }
    return sum / kN;
  }
  double InsertProbeLength(uint64_t v) const {
    const size_t capacity = slots_.size();
    assert(size_ < capacity);
 try_again:
    size_t h1 = H1(v, capacity);
    size_t total_off;
    for (size_t off = 0; off < capacity; ++off, ++total_off) {
      size_t index = h1 + off;
      bool wrapped = false;
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      Slot& slot = slots_[index];
      const Slot to_insert{wrapped, v};
      switch (slot.tag()) {
        case Tag::kEmpty: {
          return total_off;
        }
        case Tag::kPresent: {
          assert(slot.value() != v);
          if (to_insert < slot) {
            v = slot.value();
            goto try_again;


  }
#endif

  std::pair<bool, size_t/*probe_length*/> Contains(uint64_t v) const {
    // std::cout << "Finding " << ( v >> 54) << std::endl;
    const size_t capacity = slots_.size();
    size_t h1 = H1(v, capacity);
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      bool wrapped = false;
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      const Slot& slot = slots_[index];
      // std::cout << "Looking at [" << index << "]=" << slot << std::endl;
      Slot to_find{wrapped, v};
      switch (slot.tag()) {
        case Tag::kEmpty: return {false, off + 1};
        case Tag::kTombstone: {
          // If the slot > to_find it's not there.
          // If the slot == to_find it's not there
          // If slot >= to_find it's not there
          // if !(slot < to_find) it's not there.
          // std::cout << __LINE__ << ": compare " << (to_find < slot) << std::endl;
          if (! (slot < to_find)) {
            return {false, off + 1};
          }
          break;
        }
        case Tag::kPresent: {
          // std::cout << "prsent" << std::endl;
          if (slot.value() == v) {
            return {true, off + 1};
          }
          // std::cout << "not ==" << std::endl;
          if ((to_find < slot)) {
            // std::cout << to_find << " <? " << slot << std::endl;
            return {false, off + 1};
          }
          break;
        }
      }
    }
    return {false, capacity};
  }

  // A slot can be empty (nullopt), or a non-empty.  For non-empty it
  // can be present or contain a tombstone.  In the non-empty case
  // there is a bool indicating that it wrapped and a integer value.
  // The bool is there so we can compare two pairs: wrapped values
  // sort before unwrapped valueds.

  class Slot {
   public:
    enum Tag {kEmpty, kPresent, kTombstone};

    Slot() :tag_{kEmpty} {}
    Slot(bool wrapped, uint64_t v) :tag_{kPresent}, wrapped_(wrapped), value_(v) {}
    bool empty() const { return tag_ == kEmpty; }
    bool present() const { return tag_ == kPresent; }
    bool tombstone() const { return tag_ == kTombstone; }
    // Requires not empty.
    bool wrapped() const {
      assert(tag_ != kEmpty);
      return wrapped_;
    }
    uint64_t value() const {
      assert(tag_ != kEmpty);
      return value_;
    }
    Tag tag() const {
      return tag_;
    }
    void ConvertToTombstone() {
      assert(present());
      tag_ = kTombstone;
    }
   private:
    // Requires a and b not empty.
    friend bool operator<(const Slot &a, const Slot &b) {
      assert(a.tag_ != kEmpty && b.tag_ != kEmpty);
      if (a.wrapped_ != b.wrapped_)  {
        return a.wrapped_;
      }
      return a.value_ < b.value_;
    }
    friend std::ostream& operator<<(std::ostream& os, const Slot& slot) {
      if (!slot.empty()) {
        if (slot.tombstone()) {
          os << "T";
        }
        if (slot.wrapped()) {
          os << "w";
        }
        return os << PV(slot.value());
      } else {
        return os << "_";
      }
    }
    Tag tag_;
    bool wrapped_;
    uint64_t value_;
  };

  using Tag = Slot::Tag;

  friend std::ostream& operator<<(std::ostream& os, const Olp& olp) {
    os << "{size=" << olp.size_;
    size_t i = 0;
    for (const auto& slot : olp.slots_) {
      os << " ";
      if (!slot.empty()) {
        os << "[" << i << "]";
      }
      os << slot;
      ++i;
    }
    return os << "}";
  }

  // The number of elements in the table
  size_t size_ = 0;
  std::vector<Slot> slots_;
};

uint64_t RemoveRandom(std::vector<uint64_t> &values, std::mt19937_64 &gen) {
  std::uniform_int_distribution<uint64_t> distrib(0, values.size() - 1);
  uint64_t index = distrib(gen);
  assert(index < values.size());
  uint64_t v = values[index];
  values[index] = values.back();
  values.pop_back();
  return v;
}

void test() {
  std::mt19937_64 gen;
  constexpr size_t kN = 100;
  UniqueNumbers numbers(gen);
  for (double load_factor : {.95, .98, .99, .995}) {
    Olp olp{kN};
    std::vector<uint64_t> values;
    size_t size = load_factor * kN;
    for (size_t i = 0; i < size; ++i) {
      uint64_t v = numbers.Next();
      olp.insert(v);
      values.push_back(v);
      //std::cout << __LINE__ << ":after v=" << (v >> 54) << " load=" << load_factor << " table=" << olp << std::endl;
      olp.Validate();
      //std::cout << "ok" << std::endl;
    }
    // std::cout << "load=" << load_factor << " table=" << olp << std::endl;
    uint64_t v = RemoveRandom(values, gen);
    {
      bool erased = olp.erase(v);
      // std::cout << "Erased " << (v >> 54) << " Got " << olp << std::endl;
      assert(erased);
    }
    olp.insert(v);
    {
      bool erased = olp.erase(v);
      assert(erased);
    }
    olp.insert(v-1);
    // std::cout << __LINE__ << ": Inserted and Erased and inserted - 1 " << (v >> 54) << " Got " << olp << std::endl;
    olp.erase(v-1);
    // std::cout << __LINE__ << ":" << std::endl;
    olp.insert(v);
    // std::cout << __LINE__ << ": Inserted " << (v >> 54) << " Got " << olp << std::endl;
    olp.Validate();
    // std::cout << __LINE__ << ":" << std::endl;
    {
      bool erased = olp.erase(v);
      assert(erased);
    }
    olp.insert(v+1);
    // std::cout << "Inserted and Erased and inserted + 1 " << (v >> 54) << " Got " << olp << std::endl;
    olp.erase(v+1);
    olp.Validate();
  }
}

int main() {
  test();
#if 0
  std::mt19937_64 master_gen;
  constexpr size_t kN = 100;
  for (double load_factor : {.95, .98, .99, .995}) {
    std::mt19937_64 gen = master_gen;
    UniqueNumbers unique_numbers(gen);
    Olp olp{kN};
    std::vector<uint64_t> values;
    size_t size = load_factor * kN;
    size_t tenth = size / 10;
    size_t i_mod_tenth = 0;
    for (size_t i = 0; i < size; ++i) {
      uint64_t v = unique_numbers.Next();
      olp.insert(v);
      values.push_back(v);
      if (i_mod_tenth == 0) {
        std::cout << "probe lengths" << std::endl;
        ProbeLengths probelengths = olp.GetProbeLengths();
        std::cout << probelengths.found << " " << probelengths.notfound << " " << probelengths.insert << std::endl;
      }
      ++i_mod_tenth;
      if (i_mod_tenth == tenth) {
        i_mod_tenth = 0;
      }
    }
  }
#endif
}
