#ifndef _SIMPLE_INTEGER_LINEAR_PROBING_H_
#define _SIMPLE_INTEGER_LINEAR_PROBING_H_

#include <cassert>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t
#include <iostream>
#include <limits>
#include <utility>  // for swap
#include <vector>

class SimpleIntegerLinearProbing {
  // Ranges from 3/4 full to 7/8 full.
 public:
  using value_type = uint64_t;
  void reserve(size_t count);
  bool insert(uint64_t value);
  bool contains(uint64_t value) const;
  size_t size() const;
  size_t capacity() const { return 1 + slots_.size(); }
  size_t memory_estimate() const { return capacity() * sizeof(slots_[0]); }

 private:
  static constexpr size_t ceil(size_t a, size_t b) { return (a + b - 1) / b; }
  // Rehashes the table so that we can hold at least count.
  void rehash(size_t count);

  static constexpr size_t kMax = std::numeric_limits<uint64_t>::max();

  size_t occupied_slot_count_ = 0;
  size_t slot_count_ = 0;
  bool max_is_present_ = false;
  std::vector<uint64_t>
      slots_;  // slots_.size() may be bigger than slot_count_.
};

void SimpleIntegerLinearProbing::reserve(size_t count) {
  if (occupied_slot_count_ * 8 > count * 7) {
    rehash(count);
  }
}

static constexpr bool DEBUG = true;

bool SimpleIntegerLinearProbing::insert(uint64_t value) {
  if (value == kMax) {
    if (max_is_present_) {
      return false;
    }
    max_is_present_ = true;
    return true;
  }
  // If (size_ + 1) > 7*8 slot_count_ then rehash.
  if ((occupied_slot_count_ + 1) * 8 > slot_count_ * 7) {
    if (0)
      std::cerr << "osc=" << occupied_slot_count_
                << " slot_count_=" << slot_count_ << std::endl;
    // rehash to be 3/4 full
    rehash(ceil((occupied_slot_count_ + 1) * 4, 3));
  }
  size_t slot = size_t((__int128(value) * __int128(slot_count_)) >> 64);
  if (DEBUG && slot >= slots_.size()) {
    std::cerr << "Overflow 2: slot_count_=" << slot_count_ << " slot=" << slot << " value=" << value << std::endl;
    abort();
  }
  for (; slots_[slot] <= value; ++slot) {
    assert(slot < slots_.size());
    if (slots_[slot] == value) {
      return false;  // already there.
    }
    if (DEBUG && slot + 1 >= slots_.size()) {
      std::cerr << "Overflow 3" << std::endl;
      abort();
    }
  }
  while (true) {
    // Make sure that we don't overflow and also that there is one more slot
    // left over at the end (to serve as a sentinal to stop lookup).
    if (DEBUG && slot + 1 >= slots_.size()) {
      std::cerr << "Overflow" << std::endl;
      abort();
    }
    assert(slot < slots_.size());
    if (0) std::cerr << " Storing " << value << " at " << slot << std::endl;
    std::swap(value, slots_[slot]);
    if (value == kMax) {
      ++occupied_slot_count_;
      return true;
    }
    ++slot;
  }
}

void SimpleIntegerLinearProbing::rehash(size_t slot_count) {
  if (0) std::cerr << "Rehashing to " << slot_count << std::endl;
  std::vector<uint64_t> slots(slot_count + 32, kMax);
  slot_count_ = slot_count;
  std::swap(slots_, slots);
  occupied_slot_count_ = 0;
  for (uint64_t v : slots) {
    if (v != kMax) insert(v);
  }
}

bool SimpleIntegerLinearProbing::contains(uint64_t value) const {
  if (value == kMax) {
    return max_is_present_;
  }
  size_t slot = size_t((__int128(value) * __int128(slot_count_)) >> 64);
  if (DEBUG && slot >= slots_.size()) {
    std::cerr << "contains overflow" << std::endl;
    abort();
  }
  for (; slots_[slot] <= value; ++slot) {
    assert(slot < slots_.size());
    if (slots_[slot] == value) {
      return true;  // already there.
    }
    if (DEBUG && slot+1 >= slots_.size()) {
      std::cerr << "contains overflow" << std::endl;
      abort();
    }
  }
  return false;
}

size_t SimpleIntegerLinearProbing::size() const {
  return max_is_present_ + occupied_slot_count_;
}

#endif  // _SIMPLE_INTEGER_LINEAR_PROBING_H_
