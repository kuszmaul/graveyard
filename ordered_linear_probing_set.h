#ifndef _ORDERED_LINEAR_PROBING_SET_H_
#define _ORDERED_LINEAR_PROBING_SET_H_

#include <cassert>
#include <cstddef> // for size_t
#include <cstdint> // for uint64_t
#include <iostream>
#include <limits>
#include <vector>

#include "absl/container/flat_hash_set.h" // For hash_default_hash
#include "absl/log/check.h"
#include "absl/log/log.h"

// Hash Set with Ordered Linear Probing.  Type `T` must have a default
// constructor.
template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>>
class OrderedLinearProbingSet {
  // Ranges from 3/4 full to 7/8 full.
public:
  using value_type = T;
  using hasher = Hash;
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
  std::vector<T> slots_; // slots_.size() may be bigger than slot_count_.
};

template <class T, class Hash, class Eq>
void OrderedLinearProbingSet<T, Hash, Eq>::reserve(size_t count) {
  // If size * 7 /8 < count  then rehash
  if (slots_.size() * 7 < count * 8) {
    if (0)
      std::cerr << "reserve: Rehashing from " << slots_.size() << " to "
                << count << std::endl;
    rehash(count);
    if (0)
      std::cerr << "size=" << slots_.size();
  }
}

static constexpr bool kDebugOLP = true;
inline size_t PreferredSlot(size_t value, size_t slot_count) {
  return size_t((__int128(value) * __int128(slot_count)) >> 64);
}

template <class T, class Hash, class Eq>
bool OrderedLinearProbingSet<T, Hash, Eq>::insert(uint64_t value) {
  const uint64_t original_value = value;
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
  const size_t preferred_slot = PreferredSlot(hasher()(value), slot_count_);
  size_t slot = preferred_slot;
  CHECK(!kDebugOLP || slot < slots_.size())
      << "Overflow: slot_count_=" << slot_count_ << " slot=" << slot
      << " value=" << value;
  for (; slots_[slot] <= value; ++slot) {
    assert(slot < slots_.size());
    if (slots_[slot] == value) {
      return false; // already there.
    }
    CHECK(!kDebugOLP || slot + 1 < slots_.size())
        << "Overflow: slot_count_=" << slot_count_ << " slot=" << slot
        << " value=" << value;
  }
  while (true) {
    if (slot + 1 >= slots_.size()) {
      LOG(INFO) << "Pushback: slots_.size()=" << slots_.size()
                << " slot_count_=" << slot_count_
                << " occupied_slot_count_=" << occupied_slot_count_;
      slots_.push_back(kMax);
    }
    // Make sure that we don't overflow and also that there is one more slot
    // left over at the end (to serve as a sentinal to stop lookup).
    if (!(!kDebugOLP || slot + 1 < slots_.size())) {
      for (size_t i = preferred_slot; i < slots_.size(); ++i) {
        LOG(INFO) << "slots_[" << i << "]=" << slots_[i];
      }
      for (size_t i = 0; i + 1 < slots_.size(); ++i) {
        if (slots_[i] != kMax) {
          CHECK_LT(slots_[i], slots_[i + 1]);
        }
      }
    }
    CHECK(!kDebugOLP || slot + 1 < slots_.size())
        << "Overflow slot=" << slot << " slots_.size()=" << slots_.size()
        << " slot_count_=" << slot_count_
        << " original preferred_slot=" << preferred_slot << " this item is "
        << slots_[slot] << " which prefers "
        << PreferredSlot(slots_[slot], slot_count_) << " originally inserting "
        << original_value;
    assert(slot < slots_.size());
    if (0)
      std::cerr << " Storing " << value << " at " << slot << std::endl;
    std::swap(value, slots_[slot]);
    if (value == kMax) {
      ++occupied_slot_count_;
      return true;
    }
    ++slot;
  }
}

template <class T, class Hash, class Eq>
void OrderedLinearProbingSet<T, Hash, Eq>::rehash(size_t slot_count) {
  if (0)
    std::cerr << "Rehashing from " << slot_count_ << " to " << slot_count
              << std::endl;
  std::vector<uint64_t> slots(slot_count + 64, kMax);
  slot_count_ = slot_count;
  std::swap(slots_, slots);
  occupied_slot_count_ = 0;
  for (uint64_t v : slots) {
    if (v != kMax)
      insert(v);
  }
}

template <class T, class Hash, class Eq>
bool OrderedLinearProbingSet<T, Hash, Eq>::contains(uint64_t value) const {
  if (value == kMax) {
    return max_is_present_;
  }
  size_t slot = PreferredSlot(hasher()(value), slot_count_);
  CHECK(!kDebugOLP || slot < slots_.size()) << "contains overflow";
  for (; slots_[slot] <= value; ++slot) {
    assert(slot < slots_.size());
    if (slots_[slot] == value) {
      return true; // already there.
    }
    CHECK(!kDebugOLP || slot + 1 < slots_.size()) << "contains overflow";
  }
  return false;
}

template <class T, class Hash, class Eq>
size_t OrderedLinearProbingSet<T, Hash, Eq>::size() const {
  return max_is_present_ + occupied_slot_count_;
}

#endif // _ORDERED_LINEAR_PROBING_SET_H_
