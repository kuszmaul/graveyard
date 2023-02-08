#ifndef _TOMBSTONE_SET_H_
#define _TOMBSTONE_SET_H_

#include <algorithm>
#include <cassert>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t
#include <iostream>
#include <limits>
#include <vector>

#include "absl/container/flat_hash_set.h"  // For hash_default_hash
#include "absl/log/check.h"
#include "absl/log/log.h"

namespace yobiduck {

// Hash Set with Tombstone hashing.  Type `T` must have a default constructor.
// (TODO: Remove that restrictionon the default constructor).
template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>>
class TombstoneSet {
  // Ranges from 3/4 full to 7/8 full.
  static constexpr size_t kSlotsPerBucket = 14;
  static constexpr uint8_t kEmpty = 255;

 public:
  using value_type = T;

  class iterator;
  class const_iterator;

  iterator begin();
  iterator end();
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const;
  const_iterator cend() const;

  void reserve(size_t count);
  bool insert(T value);
  bool contains(const T& value) const;
  size_t size() const;
  // Returns the actual size of the table which is at least NominalSlotCount().
  size_t capacity() const { return buckets_.size() * kSlotsPerBucket; }
  size_t memory_estimate() const {
    return sizeof(*this) + buckets_.size() * sizeof(buckets_[0]);
  }

 private:
  static constexpr size_t ceil(size_t a, size_t b) { return (a + b - 1) / b; }

  // The number of slots that we are aiming for, not counting the overflow slots
  // at the end.  This value is used to compute the H1 hash (which maps from T
  // to Z/NominalSlotCount().)
  size_t NominalSlotCount() const { return bucket_count_ * kSlotsPerBucket; }

  // Preferred bucket number
  size_t H1(size_t hash) const {
    // TODO: Use the absl version.
    return size_t((__int128(hash) * __int128(bucket_count_)) >> 64);
  }

  size_t H2(size_t hash) const { return hash % 255; }

  // Returns true if the table needs rehashing to be big enough to hold
  // `target_size` elements.
  bool NeedsRehash(size_t target_size) const;

  // Rehashes the table so that we can hold at least count.
  void rehash(size_t count);

  // TODO: Make the vector be a pointer and move everything except bucket_count_
  // into the memory array.

  union Item {
    char bytes[sizeof(value_type)];
    T value;
  };
  struct Bucket {
    static constexpr uint8_t kSearchDistanceEndSentinal = 255;
    Bucket() {
      for (size_t i = 0; i < kSlotsPerBucket; ++i) h2[i] = kEmpty;
    }
    std::array<uint8_t, kSlotsPerBucket> h2;
    // The number of buckets we must search in an unsuccessful lookup that
    // starts here.
    uint8_t search_distance = 0;
    std::array<Item, kSlotsPerBucket> slots;
  };

  // The number of present items in all the buckets combined.
  size_t size_ = 0;
  size_t bucket_count_ = 0;  // For computing the hash.  The actual buckets_
                             // vector is longer so that we can overflow simply
                             // by going off the end.
  // buckets_.size() is bigger than slot_count_ (except for the case where
  // buckets_.size() == 0).
  //
  // If buckets_ is not empty then the last bucket contains
  // search_distance==kSearchDistanceEndSentinal, which is helpfor for
  // iterator++ to know when to stop scanning.
  std::vector<Bucket> buckets_;
};

template <class T, class Hash, class Eq>
bool TombstoneSet<T, Hash, Eq>::NeedsRehash(size_t target_size) const {
  return NominalSlotCount() * 7 < target_size * 8;
}

template <class T, class Hash, class Eq>
void TombstoneSet<T, Hash, Eq>::reserve(size_t count) {
  // If the nominal capacity * 7 /8 < count  then rehash
  if (NeedsRehash(count)) {
    // Set the NominalSlotCount to at least count.  Don't grow by less than 1/7.
    rehash(std::min(count, NominalSlotCount() * 8 / 7));
  }
}

// TODO: Idea, keep a bit that says whether a particular slot is out of its
// preferred bucket.  Then during insert we can avoid computing most of the
// hashes.

template <class T, class Hash, class Eq>
bool TombstoneSet<T, Hash, Eq>::insert(T value) {
  // If (size_ + 1) > 7*8 slot_count_ then rehash.
  if (NeedsRehash(size_ + 1)) {
    // rehash to be 3/4 full
    rehash(ceil((size_ + 1) * 4, 3));
  }
  // TODO: Use the Hash here and in OLP.
  const size_t preferred_bucket = H1(value);
  const size_t h2 = H2(value);
  const size_t distance = buckets_[preferred_bucket].search_distance;
  for (size_t i = 0; i <= distance; ++i) {
    assert(preferred_bucket + i < buckets_.size());
    const Bucket& bucket = buckets_[preferred_bucket + i];
    // TODO: Use vector instructions to replace this loop.
    for (size_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] == h2 && bucket.slots[j].value == value) {
        LOG(INFO) << " Already there";
        return false;
      }
    }
  }
  for (size_t i = 0; true; ++i) {
    assert(preferred_bucket + i < buckets_.size());
    Bucket& bucket = buckets_[preferred_bucket + i];
    // TODO: Use vector instructions to replace this loop.
    for (uint8_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] == kEmpty) {
        bucket.h2[j] = h2;
        // TODO: Construct in place
        bucket.slots[j].value = value;
        assert(i < Bucket::kSearchDistanceEndSentinal);
        if (i > buckets_[preferred_bucket].search_distance) buckets_[preferred_bucket].search_distance = i;
        ++size_;
        // TODO: Keep track if we are allowed to destabilize pointers, and if we
        // are, move things around to be sorted.
        return true;
      }
    }
  }
}

template <class T, class Hash, class Eq>
void TombstoneSet<T, Hash, Eq>::rehash(size_t slot_count) {
  bucket_count_ = ceil(slot_count, kSlotsPerBucket);
  assert(bucket_count_ > 0);
  // Add 4 buckets if bucket_count > 4.
  // Add 3 buckets if bucket_count == 4.
  // Add 2 buckets if bucket_count == 3.
  // Add 1 bucket if bucket_count <= 2.
  size_t extra_buckets = (bucket_count_ > 4)    ? 4
                         : (bucket_count_ <= 2) ? 1
                                                : bucket_count_ - 1;
  std::vector<Bucket> buckets(bucket_count_ + extra_buckets);
  buckets.back().search_distance = Bucket::kSearchDistanceEndSentinal;
  std::swap(buckets_, buckets);
  size_ = 0;
  for (Bucket& bucket : buckets) {
    for (size_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != kEmpty) {
        insert(std::move(bucket.slots[j].value));
        // TODO: Destruct bucket.slots[j].
      }
    }
  }
}

template <class T, class Hash, class Eq>
bool TombstoneSet<T, Hash, Eq>::contains(const T& value) const {
  if (size_ == 0) {
    return false;
  }
  const size_t preferred_bucket = H1(value);
  const size_t h2 = H2(value);
  const size_t distance = buckets_[preferred_bucket].search_distance;
  for (size_t i = 0; i <= distance; ++i) {
    assert(preferred_bucket + i < buckets_.size());
    const Bucket& bucket = buckets_[preferred_bucket + i];
    // TODO: Use vector instructions to replace this loop.
    for (size_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] == h2 && bucket.slots[j].value == value) {
        return true;
      }
    }
  }
  return false;
}

template <class T, class Hash, class Eq>
size_t TombstoneSet<T, Hash, Eq>::size() const {
  return size_;
}

template <class T, class Hash, class Eq>
typename TombstoneSet<T, Hash, Eq>::iterator
TombstoneSet<T, Hash, Eq>::begin() {
  auto it = iterator(buckets_.begin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class T, class Hash, class Eq>
typename TombstoneSet<T, Hash, Eq>::const_iterator
TombstoneSet<T, Hash, Eq>::cbegin() const {
  auto it = const_iterator(buckets_.cbegin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class T, class Hash, class Eq>
typename TombstoneSet<T, Hash, Eq>::iterator TombstoneSet<T, Hash, Eq>::end() {
  return iterator(buckets_.end(), 0);
}

template <class T, class Hash, class Eq>
typename TombstoneSet<T, Hash, Eq>::const_iterator TombstoneSet<T, Hash, Eq>::cend() const {
  return const_iterator(buckets_.cend(), 0);
}

template <class T, class Hash, class Eq>
    class TombstoneSet<T, Hash, Eq>::iterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = TombstoneSet::value_type;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  iterator() = default;
  iterator& operator++() {
    ++index_;
    return SkipEmpty();
  }

  reference operator*() {
    return bucket_->slots[index].value;
  }

 private:
  friend const_iterator;
  friend bool operator==(const iterator& a, const iterator& b) {
    return a.bucket_ == b.bucket_ && a.index_ == b.index_;
  }
  friend bool operator!=(const iterator& a, const iterator& b) {
    return !(a == b);
  }
  friend TombstoneSet;
  // index_ is allowed to be kSlotsPerBucket
  iterator& SkipEmpty() {
    while (true) {
      while (index_ < kSlotsPerBucket) {
        if (bucket_->h2[index_] != kEmpty) {
          return *this;
        }
        ++index_;
      }
      bool is_last =
          bucket_->search_distance == Bucket::kSearchDistanceEndSentinal;
      index_ = 0;
      ++bucket_;
      if (is_last) {
        // *this is the end iterator.
        return *this;
      }
    }
  }
  iterator(typename std::vector<Bucket>::iterator bucket, size_t index)
      : bucket_(bucket), index_(index) {}
  // The end iterator is represented with bucket_ == buckets_.end()
  // and index_ == kSlotsPerBucket.
  typename std::vector<Bucket>::iterator bucket_;
  size_t index_;
};

// TODO: Reduce the boilerplate
template <class T, class Hash, class Eq>
class TombstoneSet<T, Hash, Eq>::const_iterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = TombstoneSet::value_type;
  using pointer = const value_type*;
  using reference = const value_type&;
  using iterator_category = std::forward_iterator_tag;

  const_iterator() = default;
  // Implicit constructor
  const_iterator(iterator x) :bucket_(x.bucket_), index_(x.index_) {}
  const_iterator& operator++() {
    ++index_;
    return SkipEmpty();
  }

  reference operator*() {
    return bucket_->slots[index_].value;
  }

 private:
  friend bool operator==(const const_iterator& a, const const_iterator& b) {
    return a.bucket_ == b.bucket_ && a.index_ == b.index_;
  }
  friend bool operator!=(const const_iterator& a, const const_iterator& b) {
    return !(a == b);
  }
  friend TombstoneSet;
  // index_ is allowed to be kSlotsPerBucket
  const_iterator& SkipEmpty() {
    while (true) {
      while (index_ < kSlotsPerBucket) {
        if (bucket_->h2[index_] != kEmpty) {
          return *this;
        }
        ++index_;
      }
      bool is_last =
          bucket_->search_distance == Bucket::kSearchDistanceEndSentinal;
      index_ = 0;
      ++bucket_;
      if (is_last) {
        // *this is the end const_iterator.
        return *this;
      }
    }
  }
  const_iterator(typename std::vector<Bucket>::const_iterator bucket, size_t index)
      : bucket_(bucket), index_(index) {}
  // The end const_iterator is represented with bucket_ == buckets_.end()
  // and index_ == kSlotsPerBucket.
  typename std::vector<Bucket>::const_iterator bucket_;
  size_t index_;
};

}  // namespace yobiduck

#endif  // _TOMBSTONE_SET_H_
