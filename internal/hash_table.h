#ifndef _GRAVEYARD_INTERNAL_HASH_TABLE_H
#define _GRAVEYARD_INTERNAL_HASH_TABLE_H

#include <malloc.h>

#include <algorithm>
#include <new>
#include <sstream>
#include <string>
#include <utility> // for std::swap

#include "absl/container/flat_hash_set.h" // For absl::container_internal::TrailingZeros
#include "absl/log/check.h"
#include "internal/object_holder.h"

// IWYU has some strange behavior around std::swap.  It wants to get
// rid of utility and add variant. But then it's still not happy and
// keeps making changes in a cycle.

#include <array>
#include <cassert>
#include <cstddef>     // for size_t
#include <cstdint>     // for uint64_t
#include <cstdlib>     // for free, aligned_alloc
#include <iterator>    // for forward_iterator_tag, pair
#include <memory>      // for allocator_traits
#include <type_traits> // for conditional, is_same
#include <utility>     // IWYU pragma: keep
// IWYU pragma: no_include <variant>

#include "absl/log/log.h"

#if defined(__SSE2__) ||                                                       \
    (defined(_MSC_VER) &&                                                      \
     (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2)))
#define YOBIDUCK_HAVE_SSE2 1
#include <emmintrin.h>
#else
#define YOBIDUCK_HAVE_SSE2 0
#endif

// Note that we use GoogleStyleNaming for private member functions
// but cplusplus_library_standard_naming for the parts of the API that
// look like the C++ standard libary.

namespace yobiduck::internal {

static constexpr bool kHaveSse2 = (YOBIDUCK_HAVE_SSE2 != 0);

// Returns the ceiling of (a/b).
inline constexpr size_t ceil(size_t a, size_t b) { return (a + b - 1) / b; }

//  Support for heterogenous lookup
template <class, class = void>
struct IsTransparent : std::false_type {};
template <class T>
struct IsTransparent<T, absl::void_t<typename T::is_transparent>>
    : std::true_type {};

template <bool is_transparent>
struct KeyArg {
  // Transparent. Forward `K`.
  template <typename K, typename key_type>
  using type = K;
};

template <>
struct KeyArg<false> {
  // Not transparent. Always use `key_type`.
  template <typename K, typename key_type>
  using type = key_type;
};

template <class KeyType, class MappedTypeOrVoid, class Hash, class KeyEqual,
          class Allocator>
struct HashTableTraits {
  using key_type = KeyType;
  using mapped_type_or_void = MappedTypeOrVoid;
  using value_type = typename std::conditional<
      std::is_same<mapped_type_or_void, void>::value, key_type,
      std::pair<const key_type, mapped_type_or_void>>::type;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator = Allocator;
  static constexpr size_t kSlotsPerBucket = 14;

  using KeyArgImpl =
    KeyArg<IsTransparent<key_equal>::value && IsTransparent<hasher>::value>;

  // Alias used for heterogeneous lookup functions.
  // `key_arg<K>` evaluates to `K` when the functors are transparent and to
  // `key_type` otherwise. It permits template argument deduction on `K` for the
  // transparent case.
  template <class K>
  using key_arg = typename KeyArgImpl::template type<K, key_type>;

  static const key_type &KeyOf(const value_type &value) {
    if constexpr (std::is_same<mapped_type_or_void, void>::value) {
      return value;
    } else {
      return value.first;
    }
  }

  static key_type &KeyOf(value_type &value) {
    if constexpr (std::is_same<mapped_type_or_void, void>::value) {
      return value;
    } else {
      return value.first;
    }
  }

  // H2 Value for empty slots
  static constexpr uint8_t kEmpty = 255;
  static constexpr uint8_t kSearchDistanceEndSentinal = 255;
  static constexpr size_t kCacheLineSize = 64;
  static constexpr size_t kH2Modulo = 128;

  // The hash tables range from 3/4 full to 7/8 full (unless there are erase
  // operations, in which case a table might be less than 3/4 full).
  // TODO: Make these be "kConstant".
  static constexpr size_t full_utilization_numerator = 7;
  static constexpr size_t full_utilization_denominator = 8;
  static constexpr size_t rehashed_utilization_numerator = 3;
  static constexpr size_t rehashed_utilization_denominator = 4;
};

template <class Traits> union Item {
  char bytes[sizeof(typename Traits::value_type)];
  typename Traits::value_type value;
};

template <class Traits> class SortedBucketsIterator;

template <class Traits> struct Bucket {
  using key_type = typename Traits::key_type;

  template <class K>
  using key_arg = typename Traits::key_arg<K>;

  using key_equal = typename Traits::key_equal;

  // Bucket has no constructor or destructor since by default it's POD.
  Bucket() = delete;
  // Copy constructor
  Bucket(const Bucket &) = delete;
  // Copy assignment
  Bucket &operator=(Bucket other) = delete;
  // Move constructor
  Bucket(Bucket &&other) = delete;
  // Move assignment
  Bucket &operator=(Bucket &&other) = delete;
  // Destructor
  ~Bucket() = delete;

  void Init() {
    search_distance = 0;
    for (size_t i = 0; i < Traits::kSlotsPerBucket; ++i)
      h2[i] = Traits::kEmpty;
  }

  size_t PortableMatchingElements(uint8_t value) const {
    int result = 0;
    for (size_t i = 0; i < Traits::kSlotsPerBucket; ++i) {
      if (h2[i] == value) {
        result |= (1 << i);
      }
    }
    return result;
  }

  size_t MatchingElementsMask(uint8_t needle) const {
    // size_t matching = PortableMatchingElements(needle);
    if constexpr (kHaveSse2) {
      __m128i haystack =
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(&h2[0]));
      __m128i needles = _mm_set1_epi8(needle);
      int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(needles, haystack));
      mask &= (1 << Traits::kSlotsPerBucket) - 1;
      // assert(static_cast<size_t>(mask) == matching);
      return mask;
    }
    return PortableMatchingElements(needle);
  }

  template <class K = key_type>
  size_t FindElement(uint8_t needle, const key_arg<K> &key,
                     const key_equal &key_eq) const {
    size_t matches = MatchingElementsMask(needle);
    while (matches) {
      int idx = absl::container_internal::TrailingZeros(matches);
      if (key_eq(Traits::KeyOf(slots[idx].value), key)) {
        return idx;
      }
      matches &= (matches - 1);
    }
    return Traits::kSlotsPerBucket;
  }

  // Returns an empty slot number in this bucket, if it exists.  Else
  // returns Traits::kSlotsPerBucket.
  size_t FindEmpty() const {
    size_t matches;
    if constexpr (kHaveSse2 && Traits::kH2Modulo == 128) {
      static_assert(Traits::kEmpty > 128);
      // We can special case in the event that the empty value is the only H2
      // value with bit 7 set.
      __m128i h2s = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&h2[0]));
      int mask = _mm_movemask_epi8(h2s);
      mask &= (1 << Traits::kSlotsPerBucket) - 1;
      matches = mask;
    } else {
      matches = MatchingElementsMask(Traits::kEmpty);
    }
    return matches ? absl::container_internal::TrailingZeros(matches)
                   : Traits::kSlotsPerBucket;
  }

  // Returns a bit mask containing the non-empty slot numbers in this bucket.
  unsigned int FindNonEmpties() const {
    unsigned int mask;
    if constexpr (kHaveSse2 && Traits::kH2Modulo == 128) {
      static_assert(Traits::kEmpty > 128);
      // We can special case in the event that the empty value is the only H2
      // value with bit 7 set.
      __m128i h2s = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&h2[0]));
      mask = _mm_movemask_epi8(h2s);
    } else {
      mask = MatchingElementsMask(Traits::kEmpty);
    }
    mask = ~mask;
    mask &= (1 << Traits::kSlotsPerBucket) - 1;
    return mask;
  }

  std::array<uint8_t, Traits::kSlotsPerBucket> h2;
  // The number of buckets we must search in an unsuccessful lookup that starts
  // here.
  uint8_t search_distance;
  std::array<Item<Traits>, Traits::kSlotsPerBucket> slots;
};

template <class Traits> class Buckets {
public:
  // Constructs a `Buckets` with size 0 and no allocated memory.
  Buckets() = default;
  // Copy constructor
  Buckets(const Buckets &) = delete;
  // Copy assignment
  Buckets &operator=(Buckets other) = delete;
  // Move constructor
  Buckets(Buckets &&other) = delete;
  // Move assignment
  Buckets &operator=(Buckets &&other) = delete;

  ~Buckets() {
    assert((physical_size_ == 0) == (buckets_ == nullptr));
    if (buckets_ != nullptr) {
      for (Bucket<Traits> &bucket : *this) {
        for (size_t slot = 0; slot < Traits::kSlotsPerBucket; ++slot) {
          if (bucket.h2[slot] != Traits::kEmpty) {
            (&bucket.slots[slot].value)->~value_type();
          }
        }
      }
      free(buckets_);
      buckets_ = nullptr;
    }
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
    buckets_ = static_cast<Bucket<Traits> *>(aligned_alloc(
        Traits::kCacheLineSize, physical_size_ * sizeof(*buckets_)));
    assert(buckets_ != nullptr);
    for (Bucket<Traits> &bucket : *this) {
      bucket.Init();
    }
    // Set the end-of-search sentinal.
    buckets_[physical_size_ - 1].search_distance =
        Traits::kSearchDistanceEndSentinal;
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

  // The method names are lower_case (rather than CamelCase) to mimic a
  // std::vector.

  void clear() {
    logical_size_ = 0;
    physical_size_ = 0;
    free(buckets_);
    buckets_ = 0;
  }

  void swap(Buckets &other) {
    using std::swap;
    swap(logical_size_, other.logical_size_);
    swap(physical_size_, other.physical_size_);
    swap(buckets_, other.buckets_);
  }

  size_t logical_size() const { return logical_size_; }
  size_t physical_size() const { return physical_size_; }
  bool empty() const { return physical_size() == 0; }
  Bucket<Traits> &operator[](size_t index) {
    assert(index < physical_size());
    return buckets_[index];
  }
  const Bucket<Traits> &operator[](size_t index) const {
    assert(index < physical_size());
    return buckets_[index];
  }
  Bucket<Traits> *begin() { return buckets_; }
  const Bucket<Traits> *begin() const { return buckets_; }
  const Bucket<Traits> *cbegin() const { return buckets_; }
  Bucket<Traits> *end() { return buckets_ + physical_size_; }
  const Bucket<Traits> *end() const { return buckets_ + physical_size_; }
  const Bucket<Traits> *cend() const { return buckets_ + physical_size_; }

  // Returns the preferred bucket number, also known as the H1 hash.
  size_t H1(size_t hash) const {
    // TODO: Use the absl version.
    return size_t((__int128(hash) * __int128(logical_size())) >> 64);
  }

  // Returns the H2 hash, used by vector instructions to filter out most of the
  // not-equal entries.
  size_t H2(size_t hash) const { return hash % Traits::kH2Modulo; }

private:
  using value_type = typename Traits::value_type;

  // For computing the index from the hash.  The actual buckets vector is longer
  // (`physical_size_`) so that we can overflow simply by going off the end.
  size_t logical_size_ = 0;
  // The length of `buckets_`, as allocated.
  // TODO: Put the physical size into the malloced memory.
  size_t physical_size_ = 0;
  Bucket<Traits> *buckets_ = nullptr;
};

struct ProbeStatistics {
  double successful;
  double unsuccessful;
};

// The hash table
template <class Traits>
class HashTable : private ObjectHolder<'H', typename Traits::hasher>,
                  private ObjectHolder<'E', typename Traits::key_equal>,
                  private ObjectHolder<'A', typename Traits::allocator> {
private:
  using HasherHolder = ObjectHolder<'H', typename Traits::hasher>;
  using KeyEqualHolder = ObjectHolder<'E', typename Traits::key_equal>;
  using AllocatorHolder = ObjectHolder<'A', typename Traits::allocator>;

public:
  using key_type = typename Traits::key_type;
  using value_type = typename Traits::value_type;
  // This won't be exported by sets, but will be exported by maps.
  using mapped_type = typename Traits::mapped_type_or_void;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = typename Traits::hasher;
  using key_equal = typename Traits::key_equal;
  using allocator_type = typename Traits::allocator;

  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer =
      typename std::allocator_traits<allocator_type>::const_pointer;

 public:
  template <class K>
  using key_arg = typename Traits::key_arg<K>;

  HashTable();
  explicit HashTable(size_t initial_capacity, hasher const &hash = hasher(),
                     key_equal const &key_eq = key_equal(),
                     allocator_type const &allocator = allocator_type());
  // Copy constructor
  explicit HashTable(const HashTable &other);
  HashTable(const HashTable &other, const allocator_type &a);

  // Move constructor
  HashTable(HashTable &&other) : HashTable() { swap(other); }

  // Copy assignment
  HashTable &operator=(const HashTable &other) {
    clear();
    reserve(other.size());
    for (const value_type &value : other) {
      insert(value); // TODO: Optimize this given that we know `value` is not
                     // in `*this`.
    }
    return *this;
  }

  // Move assignment
  HashTable &operator=(HashTable &&other) {
    swap(other);
    return *this;
  }

 private:
  template <bool is_const> class Iterator;
 public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  iterator begin();
  iterator end();
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const;
  const_iterator cend() const;

  // node_type not implemented
  // insert_return_type not implemented

  // Note: The use of noexcept is not consistent.  Sometimes it's
  // there, but sometimes not, for now good reason.

  size_t size() const noexcept;

  bool empty() const noexcept {
    return size() == 0;
  }

  void clear();
  std::pair<iterator, bool> insert(const value_type &value);
  template<class ... Args>
  std::pair<iterator, bool> emplace(Args&&... args);
  void swap(HashTable &other) noexcept;

  // Similarly to abseil, the API of find() has two extensions.
  //
  // 1) The hash can be passed by the user.  It must be equal to the
  //    hash of the key.
  //
  // 2) Support C++20-style heterogeneous lookup.

  template <class K = key_type>
  size_t count(const key_arg<K>& key) const {
    return find(key) == end() ? 0 : 1;
  }

  template <class K = key_type>
  iterator find(const key_arg<K>& key, size_t hash);

  template <class K = key_type>
  const_iterator find(const key_arg<K>& key, size_t hash) const {
    return const_cast<HashTable*>(this)->find(key, hash);
  }

  template <class K = key_type>
  iterator find(const key_arg<K>& key) {
    PrefetchHeapBlock();
    return find(key, get_hasher_ref()(key));
  }

  template <class K = key_type>
  const_iterator find(const key_arg<K>& key) const {
    PrefetchHeapBlock();
    return find(key, get_hasher_ref()(key));
  }

  template <class K = key_type>
  bool contains(const key_arg<K>& key) const;

  template <class K = key_type>
  std::pair<iterator, iterator> equal_range(const key_arg<K>& key) {
    auto it = find(key);
    if (it != end()) return {it, std::next(it)};
    return {it, it};
  }

  template <class K = key_type>
  std::pair<iterator, iterator> equal_range(const key_arg<K>& key) const {
    auto it = find(key);
    if (it != end()) return {it, std::next(it)};
    return {it, it};
  }

  allocator_type get_allocator() const { return get_allocator_ref(); }
  allocator_type &get_allocator_ref() {
    return *static_cast<AllocatorHolder &>(*this);
  }
  const allocator_type &get_allocator_ref() const {
    return *static_cast<AllocatorHolder &>(*this);
  }
  hasher hash_function() const { return get_hasher_ref(); }
  hasher &get_hasher_ref() { return *static_cast<HasherHolder &>(*this); }
  const hasher &get_hasher_ref() const {
    return *static_cast<const HasherHolder &>(*this);
  }
  key_equal key_eq() const { return get_key_eq_ref(); }
  key_equal &get_key_eq_ref() { return *static_cast<KeyEqualHolder &>(*this); }
  const key_equal &get_key_eq_ref() const {
    return *static_cast<const KeyEqualHolder &>(*this);
  }

  // Returns the actual size of the buckets (in slots) including the
  // overflow buckets.
  size_t bucket_count() const {
    return buckets_.physical_size() * Traits::kSlotsPerBucket;
  }
  size_t capacity() const { return bucket_count(); }

  // size_t GetAllocatedMemorySize() const;
  //
  // Effect: Returns the memory allocated in this table (not including `*this`).
  size_t GetAllocatedMemorySize() const {
    return buckets_.physical_size() * sizeof(*buckets_.begin());
  }

  // Rehashes the table so that we can hold at least `count` without
  // exceeding the maximum load factor.  Also we can hold at least
  // `size()` without exceeding the maximum load factor.
  //
  // Note: If you do a series of `insert` and `rehash(0)` operations,
  // it can be slow, `rehash()` doesn't guarantee, for example, that
  // if the table grows, it grows by at least a constant factor.
  void rehash(size_t count);

  void reserve(size_t count);

  ProbeStatistics GetProbeStatistics() const;
  size_t GetSuccessfulProbeLength(const value_type &value) const;

  using hash_sorted_iterator = SortedBucketsIterator<Traits>;
  hash_sorted_iterator GetSortedBucketsIterator() {
    return hash_sorted_iterator(*this);
  }

  void Validate(int line_number = 0) const;
  void ValidateUnderDebugging(int line_number) const;
  std::string ToString() const;

private:
  // Prefetch the heap-allocated memory region to resolve potential TLB and
  // cache misses. This is intended to overlap with execution of calculating the
  // hash for a key.
  void PrefetchHeapBlock() const {
#if ABSL_HAVE_BUILTIN(__builtin_prefetch) || defined(__GNUC__)
    // This is safe even if `buckets_.begin() == nullptr`.
    __builtin_prefetch(buckets_.begin(), 0, 1);
#endif
  }

  // Checks that `*this` is valid.  Requires that a rehash or initial
  // construction has just occurred.  Specifically checks that the graveyard
  // tombstones are present.
  void CheckValidityAfterRehash(int line_number) const;

  // Returns true if the table needs rehashing to be big enough to hold
  // `target_size` elements.
  bool NeedsRehash(size_t target_size) const;

  // The number of slots that we are aiming for, not counting the overflow slots
  // at the end.  This value is used to compute the H1 hash (which maps from T
  // to Z/LogicalSlotCount().)
  size_t LogicalSlotCount() const;

  struct InsertResults {
    iterator it;
    size_t hash;
  };

  // Insert `value` into `*this`.  Requires that `value` is not already in
  // `*this` and that the table does not need rehashing.
  //
  // TODO: We can probably save the recompution of h2 if we are careful.
  template <bool keep_graveyard_tombstones>
  void InsertNoRehashNeededAndValueNotPresent(const value_type &value);

  // An overload of `InsertNoRehashNeededAndValueNotPresent` that has already
  // computed the preferred bucket and h2.
  template <bool keep_graveyard_tombstones>
  iterator InsertNoRehashNeededAndValueNotPresent(const value_type &value,
                                                  size_t preferred_bucket,
                                                  size_t h2);

  // The number of present items in all the buckets combined.
  // Todo: Put `size_` into buckets_ (in the memory).
  size_t size_ = 0;
  Buckets<Traits> buckets_;
};

template <class Traits>
HashTable<Traits>::HashTable()
    : HashTable(0, hasher(), key_equal(), allocator_type()) {}

template <class Traits>
HashTable<Traits>::HashTable(size_t initial_capacity, hasher const &hash,
                             key_equal const &key_eq,
                             allocator_type const &allocator)
    : HasherHolder(hash), KeyEqualHolder(key_eq), AllocatorHolder(allocator) {
  reserve(initial_capacity);
}

template <class Traits>
HashTable<Traits>::HashTable(const HashTable &other)
    : HashTable(other, std::allocator_traits<allocator_type>::
                           select_on_container_copy_construction(
                               get_allocator_ref())) {}

template <class Traits>
HashTable<Traits>::HashTable(const HashTable &other, const allocator_type &a)
    : HashTable(other.size(), other.get_hasher_ref(), other.get_key_eq_ref(),
                a) {
  for (const auto &v : other) {
    // TODO: We could save recomputing h2 possibly.
    InsertNoRehashNeededAndValueNotPresent<true>(v);
  }
}

template <class Traits>
typename HashTable<Traits>::iterator HashTable<Traits>::begin() {
  auto it = iterator(buckets_.begin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class Traits>
typename HashTable<Traits>::const_iterator HashTable<Traits>::cbegin() const {
  auto it = const_iterator(buckets_.cbegin(), 0);
  if (!buckets_.empty()) {
    it.SkipEmpty();
  }
  return it;
}

template <class Traits>
typename HashTable<Traits>::iterator HashTable<Traits>::end() {
  return iterator(buckets_.end(), 0);
}

template <class Traits>
typename HashTable<Traits>::const_iterator HashTable<Traits>::cend() const {
  return const_iterator(buckets_.cend(), 0);
}

template <class Traits>
template <bool is_const>
class HashTable<Traits>::Iterator {
  using original_value_type = typename Traits::value_type;
  using bucket_type = std::conditional_t<is_const, const Bucket<Traits>, Bucket<Traits>>;
public:
  using difference_type = ptrdiff_t;
  using value_type = std::conditional_t<is_const, const original_value_type, original_value_type>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;

  Iterator() = default;

  // Implicit conversion from iterator to const_iterator.
  template <bool IsConst = is_const,
            std::enable_if_t<IsConst, bool> = true>
  Iterator(iterator x) : bucket_(x.bucket_), index_(x.index_) {}

  Iterator &operator++() {
    ++index_;
    return SkipEmpty();
  }

  reference operator*() { return bucket_->slots[index_].value; }
  pointer operator->() { return &bucket_->slots[index_].value; }

private:
  using traits = Traits;

  friend const_iterator;
  friend bool operator==(const Iterator &a, const Iterator &b) {
    return a.bucket_ == b.bucket_ && a.index_ == b.index_;
  }
  friend bool operator!=(const Iterator &a, const Iterator &b) {
    return !(a == b);
  }
  friend HashTable;
  // index_ is allowed to be kSlotsPerBucket
  Iterator &SkipEmpty() {
    while (index_ < Iterator::traits::kSlotsPerBucket) {
      if (bucket_->h2[index_] != Iterator::traits::kEmpty) {
	return *this;
      }
      ++index_;
    }
    while (true) {
      unsigned int non_empties;
      while (true) {
	bool is_last =
	  bucket_->search_distance == Iterator::traits::kSearchDistanceEndSentinal;
	++bucket_;
	if (is_last) {
	  index_ = 0;
	  // *this is the end iterator.
	  return *this;
	}
	non_empties = bucket_->FindNonEmpties();
	if (non_empties != 0) {
	  break;
	}
      }
      assert(non_empties != 0);
      index_ == absl::container_internal::TrailingZeros(non_empties);
      return *this;index_ = 0;
    }
  }
  Iterator(bucket_type *bucket, size_t index)
      : bucket_(bucket), index_(index) {}
  // The end iterator is represented with bucket_ == buckets_.end()
  // and index_ == kSlotsPerBucket.
  bucket_type *bucket_;
  size_t index_;
};

template <class Traits> size_t HashTable<Traits>::size() const noexcept {
  return size_;
}

template <class Traits> void HashTable<Traits>::clear() {
  size_ = 0;
  buckets_.clear();
}

// TODO: Deal with the &&value_type insert.

template <class Traits>
std::pair<typename HashTable<Traits>::iterator, bool>
HashTable<Traits>::insert(const value_type &value) {
  if (NeedsRehash(size_ + 1)) {
    // Rehash to be, say 3/4, full.
    rehash(ceil((size_ + 1) * Traits::rehashed_utilization_denominator,
                Traits::rehashed_utilization_numerator));
  }
  // TODO: Use the Hash in OLP.
  const size_t hash = get_hasher_ref()(Traits::KeyOf(value));
  const size_t preferred_bucket = buckets_.H1(hash);
  const size_t h2 = buckets_.H2(hash);
  const size_t distance = buckets_[preferred_bucket].search_distance;
  for (size_t i = 0; i < distance; ++i) {
    // Don't use operator[], since that Buckets::operator[] has a bounds check.
    __builtin_prefetch(&(buckets_.begin() + preferred_bucket + i + 1)->h2[0]);
    assert(preferred_bucket + i < buckets_.physical_size());
    Bucket<Traits> &bucket = buckets_[preferred_bucket + i];
    size_t idx = bucket.FindElement(h2, Traits::KeyOf(value), get_key_eq_ref());
    if (idx < Traits::kSlotsPerBucket) {
      return {iterator{&bucket, idx}, false};
    }
  }
  return {InsertNoRehashNeededAndValueNotPresent<false>(value, preferred_bucket,
                                                        h2),
          true};
}

template <class Traits>
template <bool keep_graveyard_tombstones>
void HashTable<Traits>::InsertNoRehashNeededAndValueNotPresent(
    const value_type &value) {
  const key_type &key = Traits::KeyOf(value);
  const size_t hash = get_hasher_ref()(key);
  const size_t preferred_bucket = buckets_.H1(hash);
  const size_t h2 = buckets_.H2(hash);
  InsertNoRehashNeededAndValueNotPresent<keep_graveyard_tombstones>(
      value, preferred_bucket, h2);
}

void maxf(uint8_t &v1, uint8_t v2) { v1 = std::max(v1, v2); }

template <class Traits>
template <bool keep_graveyard_tombstones>
typename HashTable<Traits>::iterator
HashTable<Traits>::InsertNoRehashNeededAndValueNotPresent(
    const value_type &value, size_t preferred_bucket, size_t h2) {
  for (size_t i = 0; true; ++i) {
    assert(i < Traits::kSearchDistanceEndSentinal);
    assert(preferred_bucket + i < buckets_.physical_size());
    Bucket<Traits> &bucket = buckets_[preferred_bucket + i];
    size_t matches = bucket.MatchingElementsMask(Traits::kEmpty);
    if constexpr (keep_graveyard_tombstones) {
      // Keep the first slot free in all the odd-numbered buckets.
      if ((preferred_bucket + i) % 2 == 1) {
        assert(matches & 1ul);
        matches &= ~1ul;
      }
    }
    if (matches != 0) {
      size_t idx = absl::container_internal::TrailingZeros(matches);
      bucket.h2[idx] = h2;
      assert(h2 < Traits::kH2Modulo);
      new (&bucket.slots[idx].value) value_type(value);
      maxf(buckets_[preferred_bucket].search_distance, i + 1);
      ++size_;
      return iterator{&bucket, idx};
    }
  }
}

template <class Traits>
template<class... Args>
std::pair<typename HashTable<Traits>::iterator, bool>
HashTable<Traits>::emplace(Args&&... args) {
  // TODO: Deal with the case where we can calculate a key without constructing the value.
  return insert(value_type(args...));
}

template <class Traits>
void HashTable<Traits>::swap(HashTable &other) noexcept {
  std::swap(size_, other.size_);
  buckets_.swap(other.buckets_);
}

template <class Traits>
template <class K>
typename HashTable<Traits>::iterator HashTable<Traits>::find(const key_arg<K>& key, size_t hash) {
  if (size_ != 0) {
    const size_t preferred_bucket = buckets_.H1(hash);
    const size_t h2 = buckets_.H2(hash);
    const size_t distance = buckets_[preferred_bucket].search_distance;
    for (size_t i = 0; i <= distance; ++i) {
      // Prefetch seems to hurt lookup.  Note that F14 prefetches the entire
      // bucket up to a certain number of cache lines.
      //   __builtin_prefetch(&buckets_[preferred_bucket + i + 1].h2[0]);
      assert(preferred_bucket + i < buckets_.physical_size());
      Bucket<Traits> &bucket = buckets_[preferred_bucket + i];
      size_t idx = bucket.FindElement(h2, key, get_key_eq_ref());
      if (idx < Traits::kSlotsPerBucket) {
	return iterator{&bucket, idx};
      }
    }
  }
  return end();
}

template <class Traits>
template <class K>
bool HashTable<Traits>::contains(const key_arg<K>& value) const {
  return find(value) != end();
}

template <class Traits> std::string HashTable<Traits>::ToString() const {
  std::stringstream result;
  result << "{size=" << size_ << " logical_size=" << buckets_.logical_size()
         << " physical_size=" << buckets_.physical_size();
  for (size_t i = 0; i < buckets_.physical_size(); ++i) {
    const Bucket<Traits> *bucket = buckets_.begin() + i;
    result << std::endl
           << " bucket[" << i << "]: search_distance="
           << static_cast<size_t>(bucket->search_distance);
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      result << " " << j << ":";
      if (bucket->h2[j] == Traits::kEmpty) {
        result << "_";
      } else {
        result << bucket->slots[j].value;
      }
    }
  }
  result << "}";
  return std::move(result).str();
}

template <class Traits>
void HashTable<Traits>::Validate(int line_number) const {
  // LOG(INFO) << "Validating" << ToString();
  CHECK_LE(size(), LogicalSlotCount() * Traits::full_utilization_numerator /
                       Traits::full_utilization_denominator);
  for (size_t i = 0; i < buckets_.logical_size(); ++i) {
    // Verify that the search distances don't go off the end of the bucket
    // array.
    CHECK_LE(i + buckets_[i].search_distance, buckets_.physical_size())
        << "Search distance goes off end of of array i=" << i << " "
        << ToString();
  }
  // Verify that the overflow buckets have zero search distance, except the
  // last which has Traits::kSearchDistanceEndSentinal.
  for (size_t i = buckets_.logical_size(); i + 1 < buckets_.physical_size();
       ++i) {
    CHECK_EQ(buckets_[i].search_distance, 0);
  }
  if (buckets_.physical_size() > 0) {
    CHECK_EQ(buckets_[buckets_.physical_size() - 1].search_distance,
	     Traits::kSearchDistanceEndSentinal);
  }
  // Verify that each hashed object is a good place (not before its
  // preferred bucket or after that bucket's search distance).
  //
  // Verify that size is right.
  size_t actual_size = 0;
  for (size_t i = 0; i < buckets_.physical_size(); ++i) {
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      if (buckets_[i].h2[j] != Traits::kEmpty) {
	assert(buckets_[i].h2[j] < Traits::kH2Modulo);
	++actual_size;
	size_t hash = get_hasher_ref()(buckets_[i].slots[j].value);
	size_t h1 = buckets_.H1(hash);
	CHECK_LE(h1, i);
	CHECK_LT(h1, buckets_.logical_size());
	CHECK_LT((i - h1), buckets_[h1].search_distance)
	  << "Object is not within search distance: bucket=" << i
	  << " slot=" << j << " h1=" << h1 << " line=" << line_number
	  << " in " << ToString();
      }
    }
  }
  CHECK_EQ(actual_size, size());
}

template <class Traits>
void HashTable<Traits>::ValidateUnderDebugging(int line_number) const {
#ifndef NDEBUG
  Validate(line_number);
#endif
}

template <class Traits>
void HashTable<Traits>::CheckValidityAfterRehash(int line_number) const {
#ifndef NDEBUG
  for (size_t i = 1; i < buckets_.physical_size(); i += 2) {
    // Keep the first slot free in all the odd-numbered buckets.
    CHECK_EQ(buckets_[i].h2[0], Traits::kEmpty)
        << "bucket " << i << " from line " << line_number << " in "
        << ToString();
  }
#endif // NDEBUG
  ValidateUnderDebugging(line_number);
}

template <class Traits> void HashTable<Traits>::rehash(size_t slot_count) {
  if (slot_count == 0) {
    slot_count = ceil(size() * Traits::full_utilization_denominator,
                      Traits::full_utilization_numerator);
  }
  Buckets<Traits> buckets(ceil(slot_count, Traits::kSlotsPerBucket));
  buckets.swap(buckets_);
  size_ = 0;
  for (Bucket<Traits> &bucket : buckets) {
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != Traits::kEmpty) {
        // TODO: We could save recomputing h2 possibly.
        InsertNoRehashNeededAndValueNotPresent<true>(
            std::move(bucket.slots[j].value));
        // Note that bucket.slots[j].value will be properly destructed when we
        // leave scope, since bucket.h2[j] is not empty.
      }
    }
  }
  // CheckValidityAfterRehash(__LINE__);
}

template <class Traits> void HashTable<Traits>::reserve(size_t count) {
  if (NeedsRehash(count)) {
    size_t new_capacity_for_count =
        ceil(count * Traits::full_utilization_denominator,
             Traits::full_utilization_numerator);
    // Don't grow by less than 1/7.
    size_t new_capacity =
        std::max(new_capacity_for_count, ceil(LogicalSlotCount() * 8, 7));
    // Set the LogicalSlotCount to at least count.
    rehash(new_capacity);
  }
}

template <class Traits>
bool HashTable<Traits>::NeedsRehash(size_t target_size) const {
  return LogicalSlotCount() * Traits::full_utilization_numerator <
         target_size * Traits::full_utilization_denominator;
}

template <class Traits> size_t HashTable<Traits>::LogicalSlotCount() const {
  return buckets_.logical_size() * Traits::kSlotsPerBucket;
}

template <class Traits>
size_t
HashTable<Traits>::GetSuccessfulProbeLength(const value_type &value) const {
  const size_t h1 = buckets_.H1(get_hasher_ref()(value));
  for (size_t i = 0; i <= buckets_[h1].search_distance; ++i) {
    const Bucket<Traits> &bucket = buckets_[h1 + i];
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != Traits::kEmpty && bucket.slots[j].value == value) {
        return i + 1;
      }
    }
  }
  CHECK(false) << "Invariant failed.   value not found";
}

template <class Traits>
ProbeStatistics HashTable<Traits>::GetProbeStatistics() const {
  // Sum up the lengths for successful searches
  double success_sum = 0;
  for (const auto &value : *this) {
    success_sum += GetSuccessfulProbeLength(value);
  }
  double unsuccess_sum = 0;
  for (size_t i = 0; i < buckets_.logical_size(); ++i) {
    unsuccess_sum += buckets_[i].search_distance + 1;
  }
  return {success_sum / size(), unsuccess_sum / buckets_.logical_size()};
}

} // namespace yobiduck::internal

#endif // _GRAVEYARD_INTERNAL_HASH_TABLE_H
