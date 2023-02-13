#ifndef _GRAVEYARD_SET_H_
#define _GRAVEYARD_SET_H_

#include <malloc.h>

#include <algorithm>
#include <cassert>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t
#include <iostream>
#include <limits>
#include <vector>
#include <utility>

#include "absl/container/flat_hash_set.h"  // For hash_default_hash
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "internal/hash_table.h"

namespace yobiduck {

// Hash Set with Graveyard hashing.  Type `T` must have a default constructor.
// (TODO: Remove that restrictionon the default constructor).
template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class KeyEqual = absl::container_internal::hash_default_eq<T>,
          class Allocator = std::allocator<T>>
class GraveyardSet : 
    private yobiduck::internal::HashTable<yobiduck::internal::HashTableTraits<T, void, Hash, KeyEqual, Allocator>>
{
  using Traits = yobiduck::internal::HashTableTraits<T, void, Hash, KeyEqual, Allocator>;
  using Base = yobiduck::internal::HashTable<Traits>;

  // Ranges from 3/4 full to 7/8 full.
  static constexpr size_t kSlotsPerBucket = 14;
  static constexpr uint8_t kEmpty = 255;
  static constexpr size_t kCacheLineSize = 64;

 public:
  using typename Base::key_type;
  using typename Base::value_type;
  using typename Base::size_type;
  using typename Base::difference_type;
  using typename Base::hasher;
  using typename Base::key_equal;
  using typename Base::allocator_type;
  using typename Base::reference;
  using typename Base::const_reference;
  using typename Base::pointer;
  using typename Base::const_pointer;

  GraveyardSet() = default;

  // Copy constructor
  //  GraveyardSet(const GraveyardSet &set);
  using Base::Base;

  // Copy assignment
  GraveyardSet& operator=(const GraveyardSet& other) {
    clear();
    reserve(other.size());
    for (const T& value: other) {
      insert(value);  // TODO: Optimize this given that we know `value` is not in `*this`.
    }
  }

  // Move constructor
  GraveyardSet(GraveyardSet&& other) :GraveyardSet() {
    swap(other);
  }

  // Move assignment
  GraveyardSet& operator=(GraveyardSet&& other) {
    swap(other);
    return *this;
  }

  void clear() {
    size_ = 0;
    buckets_.Clear();
  }

  void swap(GraveyardSet& other) {
    std::swap(size_, other.size_);
    buckets_.swap(other.buckets_);
  }

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
  // Returns the actual size of the table which is at least LogicalSlotCount().
  size_t capacity() const { return buckets_.size() * kSlotsPerBucket; }
  size_t memory_estimate() const {
    return sizeof(*this) + buckets_.physical_size() * sizeof(Bucket);
  }

 private:
  static constexpr size_t ceil(size_t a, size_t b) { return (a + b - 1) / b; }

  // The number of slots that we are aiming for, not counting the overflow slots
  // at the end.  This value is used to compute the H1 hash (which maps from T
  // to Z/LogicalSlotCount().)
  size_t LogicalSlotCount() const {
    return buckets_.logical_size() * kSlotsPerBucket;
  }

  // Preferred bucket number
  size_t H1(size_t hash) const {
    // TODO: Use the absl version.
    return size_t((__int128(hash) * __int128(buckets_.logical_size())) >> 64);
  }

  size_t H2(size_t hash) const { return hash % 255; }

  // Returns true if the table needs rehashing to be big enough to hold
  // `target_size` elements.
  bool NeedsRehash(size_t target_size) const;

  // Rehashes the table so that we can hold at least count.
  void rehash(size_t count);

  union Item {
    char bytes[sizeof(value_type)];
    T value;
  };
  struct Bucket {
    static constexpr uint8_t kSearchDistanceEndSentinal = 255;

    // Buckets aren't constructed, since the values inside are constructed in
    // place.
    Bucket() = delete;
    // Copy constructor
    Bucket(const Bucket&) = delete;
    // Copy assignment
    Bucket& operator=(Bucket other) = delete;
    // Move constructor
    Bucket(Bucket&& other) = delete;
    // Move assignment
    Bucket& operator=(Bucket&& other) = delete;
    // Destructor
    ~Bucket() = delete;


    void Init() {
      for (size_t i = 0; i < kSlotsPerBucket; ++i) h2[i] = kEmpty;
      search_distance = 0;
    }
    std::array<uint8_t, kSlotsPerBucket> h2;
    // The number of buckets we must search in an unsuccessful lookup that
    // starts here.
    uint8_t search_distance;
    std::array<Item, kSlotsPerBucket> slots;
  };
  class Buckets {
   public:
    // Constructs a `Buckets` with size 0 and no allocated memory.
    Buckets() = default;
    // Copy constructor
    Buckets(const Buckets&) = delete;
    // Copy assignment
    Buckets& operator=(Buckets other) = delete;
    // Move constructor
    Buckets(Buckets&& other) = delete;
    // Move assignment
    Buckets& operator=(Buckets&& other) = delete;

    void Clear() {
      logical_size_ = 0;
      physical_size_ = 0;
      free(buckets_);
      buckets_ = 0;
    }

    // Constructs a `Buckets` that has the given logical bucket size (which must
    // be positive).
    explicit Buckets(size_t logical_size) : logical_size_(logical_size) {
      assert(logical_size_ > 0);
      // Add 4 buckets if logical_size_ > 4.
      // Add 3 buckets if logical_size_ == 4.
      // Add 2 buckets if logical_size_ == 3.
      // Add 1 bucket if logical_bucket_count_ <= 2.
      size_t extra_buckets = (logical_size_ > 4)    ? 4
                             : (logical_size_ <= 2) ? 1
                                                    : logical_size_ - 1;
      physical_size_ = logical_size_ + extra_buckets;
      assert(physical_size_ > 0);
      // TODO: Round up the physical_bucket_size_ to the actual size allocated.
      // To do this we can call malloc_usable_size to find out how big it really
      // is.  But we aren't supposed to modify those bytes (it will mess up
      // tools such as address sanitizer or valgrind).  So we realloc the
      // pointer to the actual size.
      buckets_ = static_cast<Bucket*>(
          aligned_alloc(kCacheLineSize, physical_size_ * sizeof(*buckets_)));
      assert(buckets_ != nullptr);
      for (Bucket& bucket : *this) {
        bucket.Init();
      }
      // Set the end-of-search sentinal.
      buckets_[physical_size_ - 1].search_distance =
          Bucket::kSearchDistanceEndSentinal;
      if (0) {
        // It turns out that for libc malloc, the extra usable size usually just
        // 8 extra bytes.
        size_t allocated = malloc_usable_size(buckets_);
        LOG(INFO) << "logical_size=" << logical_size_
                  << " physical_size=" << physical_size_
                  << " allocated bytes=" << allocated
                  << " size=" << allocated / sizeof(*buckets_) << " %"
                  << sizeof(*buckets_) << "=" << allocated % sizeof(*buckets_);
      }
    }
    ~Buckets() {
      assert((physical_size_ == 0) == (buckets_ == nullptr));
      if (buckets_ != nullptr) {
        // TODO: Destruct the nonempty slots
        free(buckets_);
        buckets_ = nullptr;
      }
    }
    void swap(Buckets& other) {
      using std::swap;
      swap(logical_size_, other.logical_size_);
      swap(physical_size_, other.physical_size_);
      swap(buckets_, other.buckets_);
    }
    size_t logical_size() const { return logical_size_; }
    size_t physical_size() const { return physical_size_; }
    bool empty() const { return physical_size_ == 0; }
    Bucket& operator[](size_t index) {
      assert(index < physical_size_);
      return buckets_[index];
    }
    const Bucket& operator[](size_t index) const {
      assert(index < physical_size_);
      return buckets_[index];
    }
    Bucket* begin() { return buckets_; }
    const Bucket* begin() const { return buckets_; }
    const Bucket* cbegin() const { return buckets_; }
    Bucket* end() { return buckets_ + physical_size_; }
    const Bucket* end() const { return buckets_ + physical_size_; }
    const Bucket* cend() const { return buckets_ + physical_size_; }

   private:
    // For computing the index from the hash.  The actual buckets vector is
    // longer (`physical_size_`) so that we can overflow simply by going off the
    // end.
    size_t logical_size_ = 0;
    // The length of `buckets_`, as allocated.
    // TODO: Put the physical size into the malloced memory.
    size_t physical_size_ = 0;
    Bucket* buckets_ = nullptr;
  };

  // The number of present items in all the buckets combined.
  // Todo: Put `size_` into buckets_.
  size_t size_ = 0;
  Buckets buckets_;
};

template <class T, class Hash, class Eq, class Allocator>
bool GraveyardSet<T, Hash, Eq, Allocator>::NeedsRehash(size_t target_size) const {
  return LogicalSlotCount() * 7 < target_size * 8;
}

template <class T, class Hash, class Eq, class Allocator>
void GraveyardSet<T, Hash, Eq, Allocator>::reserve(size_t count) {
  // If the logical capacity * 7 /8 < count  then rehash
  if (NeedsRehash(count)) {
    // Set the LogicalSlotCount to at least count.  Don't grow by less than 1/7.
    rehash(std::max(count, LogicalSlotCount() * 8 / 7));
  }
}

// TODO: Idea, keep a bit that says whether a particular slot is out of its
// preferred bucket.  Then during insert we can avoid computing most of the
// hashes.

template <class T, class Hash, class Eq, class Allocator>
bool GraveyardSet<T, Hash, Eq, Allocator>::insert(T value) {
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
    assert(preferred_bucket + i < buckets_.physical_size());
    const Bucket& bucket = buckets_[preferred_bucket + i];
    // TODO: Use vector instructions to replace this loop.
    for (size_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] == h2 && bucket.slots[j].value == value) {
        LOG(INFO) << " Already there in bucket " << preferred_bucket + i;
        return false;
      }
    }
  }
  for (size_t i = 0; true; ++i) {
    assert(preferred_bucket + i < buckets_.physical_size());
    Bucket& bucket = buckets_[preferred_bucket + i];
    // TODO: Use vector instructions to replace this loop.
    for (uint8_t j = 0; j < kSlotsPerBucket; ++j) {
      if (bucket.h2[j] == kEmpty) {
        bucket.h2[j] = h2;
        // TODO: Construct in place
        bucket.slots[j].value = value;
        assert(i < Bucket::kSearchDistanceEndSentinal);
        if (i > buckets_[preferred_bucket].search_distance)
          buckets_[preferred_bucket].search_distance = i;
        ++size_;
        // TODO: Keep track if we are allowed to destabilize pointers, and if we
        // are, move things around to be sorted.
        return true;
      }
    }
  }
}

template <class T, class Hash, class Eq, class Allocator>
void GraveyardSet<T, Hash, Eq, Allocator>::rehash(size_t slot_count) {
  Buckets buckets(ceil(slot_count, kSlotsPerBucket));
  buckets.swap(buckets_);
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

template <class T, class Hash, class Eq, class Allocator>
bool GraveyardSet<T, Hash, Eq, Allocator>::contains(const T& value) const {
  if (size_ == 0) {
    return false;
  }
  const size_t preferred_bucket = H1(value);
  const size_t h2 = H2(value);
  const size_t distance = buckets_[preferred_bucket].search_distance;
  for (size_t i = 0; i <= distance; ++i) {
    assert(preferred_bucket + i < buckets_.physical_size());
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

template <class T, class Hash, class Eq, class Allocator>
size_t GraveyardSet<T, Hash, Eq, Allocator>::size() const {
  return size_;
}

template <class T, class Hash, class Eq, class Allocator>
typename GraveyardSet<T, Hash, Eq, Allocator>::iterator
GraveyardSet<T, Hash, Eq, Allocator>::begin() {
  auto it = iterator(buckets_.begin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class T, class Hash, class Eq, class Allocator>
typename GraveyardSet<T, Hash, Eq, Allocator>::const_iterator
GraveyardSet<T, Hash, Eq, Allocator>::cbegin() const {
  auto it = const_iterator(buckets_.cbegin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class T, class Hash, class Eq, class Allocator>
typename GraveyardSet<T, Hash, Eq, Allocator>::iterator GraveyardSet<T, Hash, Eq, Allocator>::end() {
  return iterator(buckets_.end(), 0);
}

template <class T, class Hash, class Eq, class Allocator>
typename GraveyardSet<T, Hash, Eq, Allocator>::const_iterator
GraveyardSet<T, Hash, Eq, Allocator>::cend() const {
  return const_iterator(buckets_.cend(), 0);
}

template <class T, class Hash, class Eq, class Allocator>
class GraveyardSet<T, Hash, Eq, Allocator>::iterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = GraveyardSet::value_type;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  iterator() = default;
  iterator& operator++() {
    ++index_;
    return SkipEmpty();
  }

  reference operator*() { return bucket_->slots[index].value; }

 private:
  friend const_iterator;
  friend bool operator==(const iterator& a, const iterator& b) {
    return a.bucket_ == b.bucket_ && a.index_ == b.index_;
  }
  friend bool operator!=(const iterator& a, const iterator& b) {
    return !(a == b);
  }
  friend GraveyardSet;
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
  iterator(Bucket* bucket, size_t index) : bucket_(bucket), index_(index) {}
  // The end iterator is represented with bucket_ == buckets_.end()
  // and index_ == kSlotsPerBucket.
  Bucket* bucket_;
  size_t index_;
};

// TODO: Reduce the boilerplate
template <class T, class Hash, class Eq, class Allocator>
class GraveyardSet<T, Hash, Eq, Allocator>::const_iterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = GraveyardSet::value_type;
  using pointer = const value_type*;
  using reference = const value_type&;
  using iterator_category = std::forward_iterator_tag;

  const_iterator() = default;
  // Implicit constructor
  const_iterator(iterator x) : bucket_(x.bucket_), index_(x.index_) {}
  const_iterator& operator++() {
    ++index_;
    return SkipEmpty();
  }

  reference operator*() { return bucket_->slots[index_].value; }

 private:
  friend bool operator==(const const_iterator& a, const const_iterator& b) {
    return a.bucket_ == b.bucket_ && a.index_ == b.index_;
  }
  friend bool operator!=(const const_iterator& a, const const_iterator& b) {
    return !(a == b);
  }
  friend GraveyardSet;
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
  const_iterator(const Bucket* bucket, size_t index)
      : bucket_(bucket), index_(index) {}
  // The end const_iterator is represented with bucket_ == buckets_.end()
  // and index_ == kSlotsPerBucket.
  const Bucket* bucket_;
  size_t index_;
};

}  // namespace yobiduck

#endif  // _TOMBSTONE_SET_H_
