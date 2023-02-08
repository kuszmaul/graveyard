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

  iterator begin();
  iterator end();

  void reserve(size_t count);
  bool insert(T value);
  bool contains(const T& value) const;
  size_t size() const;
  // Returns the actual size of the table which is at least NominalSlotCount().
  size_t capacity() const { return buckets_.size() * kSlotsPerBucket; }
  size_t memory_estimate() const { return sizeof(*this) + buckets_.size() * sizeof(buckets_[0]); }

 private:
  static constexpr size_t ceil(size_t a, size_t b) { return (a + b - 1) / b; }

  // The number of slots that we are aiming for, not counting the overflow slots
  // at the end.  This value is used to compute the H1 hash (which maps from T
  // to Z/NominalSlotCount().)
  size_t NominalSlotCount() const {
    return bucket_count_ * kSlotsPerBucket;
  }

  // Preferred bucket number
  size_t H1(size_t hash) const {
    // TODO: Use the absl version.
    return size_t((__int128(hash) * __int128(bucket_count_)) >> 64);
  }

  size_t H2(size_t hash) const {
    return hash % 255;
  }

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
  size_t bucket_count_ = 0; // For computing the hash.  The actual buckets_
                            // vector is longer so that we can overflow simply
                            // by going off the end.
  std::vector<Bucket> buckets_;  // slots_.size() may be bigger than slot_count_.
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
  LOG(INFO) << "Insert " << value;
  if (NeedsRehash(size_ + 1)) {
    LOG(INFO) << "Needs rehash";
    // rehash to be 3/4 full
    rehash(ceil((size_ + 1) * 4, 3));
    LOG(INFO) << "rehashed to bucket_count_=" << bucket_count_;
  }
  // TODO: Use the Hash here and in OLP.
  const size_t preferred_bucket = H1(value);
  const size_t h2 = H2(value);
  const size_t distance = buckets_[preferred_bucket].search_distance;
  LOG(INFO) << "h2=" << h2 << " distance=" << distance;
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
        bucket.search_distance = std::max(bucket.search_distance, j);
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
  std::vector<Bucket> buckets(bucket_count_ + std::min(4ul, bucket_count_ - 1ul));
  LOG(INFO) << " rehashed: new slot_count=" << slot_count << " bucket_count_=" << bucket_count_;
  std::swap(buckets_, buckets);
  size_ = 0;
  for (Bucket& bucket : buckets) {
    for (size_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != kEmpty) {
        LOG(INFO) << "Moving " << bucket.slots[j].value;
        insert(std::move(bucket.slots[j].value));
        // TODO: Destruct bucket.slots[j].
      }
    }
  }
}

template <class T, class Hash, class Eq>
bool TombstoneSet<T, Hash, Eq>::contains(const T& value) const {
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
class TombstoneSet<T, Hash, Eq>::iterator {
};

}  // namespace yobiduck

#endif  // _TOMBSTONE_SET_H_
