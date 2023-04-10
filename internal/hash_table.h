#ifndef _GRAVEYARD_INTERNAL_HASH_TABLE_H
#define _GRAVEYARD_INTERNAL_HASH_TABLE_H

#include <malloc.h>
#include <sys/mman.h>

#include <algorithm>
#include <cstdio>
#include <new>
#include <sstream>
#include <string>
#include <utility> // for std::swap

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

// Return the number of trailing zeros
template <typename T> static constexpr uint32_t CountTrailingZeros(T x) {
  assert(x != 0);
  static_assert(sizeof(x) == sizeof(unsigned int) ||
                sizeof(x) == sizeof(unsigned long long));
  if constexpr (sizeof(x) == 4) {
    return __builtin_ctz(x);
  } else {
    return __builtin_ctzll(x);
  }
}

//  Support for heterogenous lookup
template <class, class = void> struct IsTransparent : std::false_type {};
template <class T>
struct IsTransparent<T, std::void_t<typename T::is_transparent>>
    : std::true_type {};

template <bool is_transparent> struct KeyArg {
  // Transparent. Forward `K`.
  template <typename K, typename key_type> using type = K;
};

template <> struct KeyArg<false> {
  // Not transparent. Always use `key_type`.
  template <typename K, typename key_type> using type = key_type;
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

  // When rehashing, add a tombstone every `kTombstonePeriod` slots.
  // Set this to `std::limits<size_t>::max()` for no tombstones.
  static constexpr std::optional<size_t> kTombstonePeriod = 28;
};

template <class Traits> struct alignas(typename Traits::value_type) Item {
  char bytes[sizeof(typename Traits::value_type)];
  // typename Traits::value_type value;
  typename Traits::value_type &value() {
    return *reinterpret_cast<typename Traits::value_type *>(&bytes[0]);
  }
  const typename Traits::value_type &value() const {
    return *reinterpret_cast<const typename Traits::value_type *>(&bytes[0]);
  }
};

template <class Traits> class SortedBucketsIterator;

template <class Traits> struct Bucket {
  using key_type = typename Traits::key_type;

  template <class K> using key_arg = typename Traits::template key_arg<K>;

  using key_equal = typename Traits::key_equal;

  // Trivial constructor, copyconstructor, copy assignment, move
  // consgtructor, move assignment, and destructor.

  void Init() {
    static_assert(std::is_trivial<Bucket>::value, "Bucket should be POD");
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
      int idx = CountTrailingZeros(matches);
      if (key_eq(Traits::KeyOf(slots[idx].value()), key)) {
        return idx;
      }
      matches &= (matches - 1);
    }
    return Traits::kSlotsPerBucket;
  }

  // Returns an integer bitmask indicating which slots are empty.
  unsigned int FindEmpties() const {
    if constexpr (kHaveSse2 && Traits::kH2Modulo == 128) {
      static_assert(Traits::kEmpty > 128);
      // We can special case in the event that the empty value is the only H2
      // value with bit 7 set.
      __m128i h2s = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&h2[0]));
      unsigned int empties = _mm_movemask_epi8(h2s);
      empties &= (1 << Traits::kSlotsPerBucket) - 1;
      return empties;
    } else {
      return MatchingElementsMask(Traits::kEmpty);
    }
  }

  // Returns an empty slot number in this bucket, if it exists.  Else
  // returns Traits::kSlotsPerBucket.
  size_t FindEmpty() const {
    unsigned int empties = FindEmpties();
    return empties ? CountTrailingZeros(empties) : Traits::kSlotsPerBucket;
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

  // Deallocate the memory in this.  Requires that none of the slots
  // contain values.
  void Deallocate() {
    free(data_);
    data_ = nullptr;
    logical_size_ = 0;
  }

  ~Buckets() { clear(); }

  // Constructs a `Buckets` that has the given logical bucket size (which must
  // be positive).
  //
  // The buckets aren't initialized.
  explicit Buckets(size_t logical_size) : logical_size_(logical_size) {
    assert(logical_size_ > 0);
    size_t physical = physical_size();
    // TODO: Round up the physical_bucket_size_ to the actual size allocated.
    // To do this we can call malloc_usable_size to find out how big it really
    // is.  But we aren't supposed to modify those bytes (it will mess up
    // tools such as address sanitizer or valgrind).  So we realloc the
    // pointer to the actual size.
    data_ = static_cast<char *>(std::aligned_alloc(
        Traits::kCacheLineSize, physical * sizeof(Bucket<Traits>)));
    assert(data_ != nullptr);
    if (0) {
      // It turns out that for libc malloc, the extra usable size usually just
      // 8 extra bytes.
      size_t allocated = malloc_usable_size(data_);
      LOG(INFO) << "logical_size=" << logical_size_
                << " physical_size=" << physical
                << " allocated bytes=" << allocated
                << " size=" << allocated / sizeof(Bucket<Traits>) << " %"
                << sizeof(Bucket<Traits>) << "="
                << allocated % sizeof(Bucket<Traits>);
    }
  }

  // The method names are lower_case (rather than CamelCase) to mimic a
  // std::vector.

  void clear() {
    assert((logical_size() == 0) == (data_ == nullptr));
    if (data_ != nullptr) {
      for (Bucket<Traits> &bucket : *this) {
        for (size_t slot = 0; slot < Traits::kSlotsPerBucket; ++slot) {
          if (bucket.h2[slot] != Traits::kEmpty) {
            bucket.slots[slot].value().~value_type();
          }
        }
      }
      free(data_);
      data_ = nullptr;
    }
    logical_size_ = 0;
  }

  void swap(Buckets &other) {
    using std::swap;
    swap(logical_size_, other.logical_size_);
    swap(data_, other.data_);
  }

  size_t logical_size() const { return logical_size_; }
  size_t physical_size() const {
    // Add 5 buckets if logical_size_ >= 6.
    // Add 4 buckets if logical_size_ == 5.
    // Add 3 buckets if logical_size_ == 4.
    // Add 2 buckets if logical_size_ == 3.
    // Add 1 bucket if logical_size_ from 1 to 2.
    // Add 0 bucketrs if logical_size_ == 0;
    // TODO: We'd like to add 0 buckets if the logical_bucket_count == 1.
    size_t extra_buckets = (logical_size_ >= 6)  ? 5
                           : (logical_size_ > 2) ? logical_size_ - 1
                           : (logical_size_ > 0) ? 1
                                                 : 0;
    return logical_size() + extra_buckets;
  }
  bool empty() const { return logical_size() == 0; }
  Bucket<Traits> &operator[](size_t index) {
    assert(index < physical_size());
    return begin()[index];
  }
  const Bucket<Traits> &operator[](size_t index) const {
    assert(index < physical_size());
    return cbegin()[index];
  }
  Bucket<Traits> *begin() { return const_cast<Bucket<Traits> *>(cbegin()); }
  const Bucket<Traits> *begin() const { return cbegin(); }
  const Bucket<Traits> *cbegin() const {
    return static_cast<const Bucket<Traits> *>(
        static_cast<const void *>(data_ + buckets_offset));
  }
  Bucket<Traits> *end() { return begin() + physical_size(); }
  const Bucket<Traits> *end() const { return cend(); }
  const Bucket<Traits> *cend() const { return cbegin() + physical_size(); }

  // Returns the preferred bucket number, also known as the H1 hash.
  size_t H1(size_t hash) const {
    // TODO: Use the absl version.
    return size_t((__int128(hash) * __int128(logical_size())) >> 64);
  }

  // Returns the H2 hash, used by vector instructions to filter out most of the
  // not-equal entries.
  size_t H2(size_t hash) const { return hash % Traits::kH2Modulo; }

private:
  static constexpr size_t buckets_offset = 0;

  using value_type = typename Traits::value_type;

  // For computing the index from the hash.  The actual buckets vector is longer
  // (`physical_size_`) so that we can overflow simply by going off the end.
  //
  // We keep `logical_size_` here the header, and everything else in the data_.
  size_t logical_size_ = 0;
  char *data_ = nullptr;
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
  template <class K> using key_arg = typename Traits::template key_arg<K>;

  HashTable();
  explicit HashTable(size_t initial_capacity, hasher const &hash = hasher(),
                     key_equal const &key_eq = key_equal(),
                     allocator_type const &allocator = allocator_type());
  // Copy constructor
  explicit HashTable(const HashTable &other);
  HashTable(const HashTable &other, const allocator_type &a);

  // Move constructor
  HashTable(HashTable &&other) : HashTable() { swap(other); }

  // Copy assignment not using copy-and-swap idiom.
  HashTable &operator=(const HashTable &other);

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

  bool empty() const noexcept { return size() == 0; }

  void clear();
  std::pair<iterator, bool> insert(const value_type &value);
  template <class... Args> std::pair<iterator, bool> emplace(Args &&...args);
  // Note: As for absl, this overload doesn't return an iterator.
  // If that iterator is needed, simply post increment the iterator:
  //
  //     set.erase(it++);
  void erase(iterator pos);
  void erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);
  template <class K = key_type> size_t erase(const key_arg<K> &key);
  void swap(HashTable &other) noexcept;

  // Similarly to abseil, the API of find() has two extensions.
  //
  // 1) The hash can be passed by the user.  It must be equal to the
  //    hash of the key.
  //
  // 2) Support C++20-style heterogeneous lookup.

  template <class K = key_type> size_t count(const key_arg<K> &key) const {
    return find(key) == end() ? 0 : 1;
  }

  template <class K = key_type>
  iterator find(const key_arg<K> &key, size_t hash);

  template <class K = key_type>
  const_iterator find(const key_arg<K> &key, size_t hash) const {
    return const_cast<HashTable *>(this)->find(key, hash);
  }

  template <class K = key_type> iterator find(const key_arg<K> &key) {
    PrefetchHeapBlock();
    return find(key, get_hasher_ref()(key));
  }

  template <class K = key_type>
  const_iterator find(const key_arg<K> &key) const {
    PrefetchHeapBlock();
    return find(key, get_hasher_ref()(key));
  }

  template <class K = key_type> bool contains(const key_arg<K> &key) const;

  template <class K = key_type>
  std::pair<iterator, iterator> equal_range(const key_arg<K> &key) {
    auto it = find(key);
    if (it != end())
      return {it, std::next(it)};
    return {it, it};
  }

  template <class K = key_type>
  std::pair<iterator, iterator> equal_range(const key_arg<K> &key) const {
    auto it = find(key);
    if (it != end())
      return {it, std::next(it)};
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
    // This is safe even if `buckets_.begin() == nullptr`.

    // Portability note: This should be #ifdef'd away for systems that
    // don't have `__builtin_prefetch`.
    __builtin_prefetch(buckets_.begin(), 0, 1);
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

  // Inserts `value` into `*this`.  Requires that `value` is not
  // already in `*this` and that the table does not need rehashing.
  // We don't maintain tombstones.
  //
  // TODO: We can probably save the recompution of h2 if we are careful.
  void InsertNoRehashNeededAndValueNotPresent(const value_type &value);

  // An overload of `InsertNoRehashNeededAndValueNotPresent` that has already
  // computed the preferred bucket and h2.
  iterator InsertNoRehashNeededAndValueNotPresent(const value_type &value,
                                                  size_t preferred_bucket,
                                                  size_t h2);

  // Inserts a value.  Works well if the values are inserted in
  // (roughly) ascending order.  All the buckets starting at, and
  // following, `first_uninitialized_bucket` are uninitialized.  Used
  // during rehash (and TODO: use during copy).  Doesn't update `size_`.
  template <bool insert_tombstones>
  void InsertAscending(value_type value,
                       size_t &first_uninitialized_bucket);

  // Finishes the rehash or copy by initializing all the remaining
  // uninitialized buckets.
  void FinishInsertAscending(size_t first_uninitialized_bucket);

  // Arrange for the values in `buckets` to be inserted into `*this`
  // (incrementing `size_` for every insert).
  //
  // Requires: `*this` is empty.  Thus, this code can assume that
  // there are no duplicates in `buckets`.
  //
  // If `is_rehash` then buckets is destroyed.  In that case, this
  // code may move the values from `buckets` instead of copying them.
  void RehashFrom(Buckets<Traits> &buckets);
  void CopyFrom(const Buckets<Traits> &buckets);

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
  // Tombstones help future inserts.  We don't insert graveyard
  // tombstones, since, having sized `*this` to be just right, we
  // won't be able to do any more inserts with rehashing anyway.
  //
  // TODO: We could conceivably squeeze the table even more, and
  // reduce the table size by the number of tombstones we didn't
  // place
  size_ = other.size_;
  CopyFrom(other.buckets_);
}

template <class Traits>
HashTable<Traits>& HashTable<Traits>::operator=(const HashTable &other)
{
  clear();
  reserve(other.size_);
  size_ = other.size_;
  CopyFrom(other.buckets_);
  return *this;
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
  using bucket_type =
      std::conditional_t<is_const, const Bucket<Traits>, Bucket<Traits>>;

public:
  using difference_type = ptrdiff_t;
  using value_type = std::conditional_t<is_const, const original_value_type,
                                        original_value_type>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;

  Iterator() = default;

  // Implicit conversion from iterator to const_iterator.
  template <bool IsConst = is_const, std::enable_if_t<IsConst, bool> = true>
  Iterator(const iterator &x) : bucket_(x.bucket_), index_(x.index_) {}

  Iterator &operator++() {
    ++index_;
    return SkipEmpty();
  }

  Iterator operator++(int) {
    Iterator result = *this;
    ++*this;
    return result;
    ;
  }

  reference operator*() { return bucket_->slots[index_].value(); }
  pointer operator->() { return &bucket_->slots[index_].value(); }

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
    // Look for a non-empty value, starting at index_ in the current bucket.
    {
      unsigned int non_empties = bucket_->FindNonEmpties();
      non_empties &= ~((1u << index_) - 1);
      if (non_empties != 0) {
        index_ = CountTrailingZeros(non_empties);
        return *this;
      }
    }
    // There were none, so look for a bucket with at least one
    // non-empty value, and set `index_` accordingly.
    while (true) {
      bool is_last = bucket_->search_distance ==
                     Iterator::traits::kSearchDistanceEndSentinal;
      ++bucket_;
      if (is_last) {
        index_ = 0;
        // *this is the end iterator.
        return *this;
      }
      unsigned int non_empties = bucket_->FindNonEmpties();
      if (non_empties != 0) {
        index_ = CountTrailingZeros(non_empties);
        return *this;
      }
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
  return {InsertNoRehashNeededAndValueNotPresent(value, preferred_bucket,
                                                 h2),
          true};
}

template <class Traits>
void HashTable<Traits>::InsertNoRehashNeededAndValueNotPresent(
    const value_type &value) {
  const key_type &key = Traits::KeyOf(value);
  const size_t hash = get_hasher_ref()(key);
  const size_t preferred_bucket = buckets_.H1(hash);
  const size_t h2 = buckets_.H2(hash);
  InsertNoRehashNeededAndValueNotPresent(value, preferred_bucket, h2);
}

void maxf(uint8_t &v1, uint8_t v2) { v1 = std::max(v1, v2); }

template <class Traits>
typename HashTable<Traits>::iterator
HashTable<Traits>::InsertNoRehashNeededAndValueNotPresent(
    const value_type &value, size_t preferred_bucket, size_t h2) {
  for (size_t i = 0; true; ++i) {
    assert(i < Traits::kSearchDistanceEndSentinal);
    assert(preferred_bucket + i < buckets_.physical_size());
    Bucket<Traits> &bucket = buckets_[preferred_bucket + i];
    // TODO: Use FindEmpty
    size_t matches = bucket.MatchingElementsMask(Traits::kEmpty);
    if (matches != 0) {
      size_t idx = CountTrailingZeros(matches);
      bucket.h2[idx] = h2;
      assert(h2 < Traits::kH2Modulo);
      new (&bucket.slots[idx].value()) value_type(value);
      maxf(buckets_[preferred_bucket].search_distance, i + 1);
      ++size_;
      return iterator{&bucket, idx};
    }
  }
}

template <class Traits>
template <class... Args>
std::pair<typename HashTable<Traits>::iterator, bool>
HashTable<Traits>::emplace(Args &&...args) {
  // TODO: Deal with the case where we can calculate a key without constructing
  // the value.
  return insert(value_type(args...));
}

template <class Traits>
void HashTable<Traits>::swap(HashTable &other) noexcept {
  std::swap(size_, other.size_);
  buckets_.swap(other.buckets_);
}

template <class Traits> void HashTable<Traits>::erase(iterator pos) {
  erase(const_iterator(pos));
}

template <class Traits> void HashTable<Traits>::erase(const_iterator pos) {
  Bucket<Traits> *bucket = const_cast<Bucket<Traits> *>(pos.bucket_);
  size_t index = pos.index_;
  // We can assume that it's a valid iterator.
  assert(bucket->h2[index] != Traits::kEmpty);
  assert(size_ > 0);
  bucket->h2[index] = Traits::kEmpty;
  bucket->slots[index].value().~value_type();
  --size_;
  return;
}

template <class Traits>
typename HashTable<Traits>::iterator
HashTable<Traits>::erase(const_iterator first, const_iterator last) {
  while (first != last) {
    erase(first++);
  }
  return iterator(const_cast<Bucket<Traits> *>(last.bucket_), last.index_);
}

template <class Traits>
template <class K>
size_t HashTable<Traits>::erase(const key_arg<K> &key) {
  auto it = find(key);
  if (it == end()) {
    return 0;
  }
  erase(it);
  return 1;
}

template <class Traits>
template <class K>
typename HashTable<Traits>::iterator
HashTable<Traits>::find(const key_arg<K> &key, size_t hash) {
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
bool HashTable<Traits>::contains(const key_arg<K> &value) const {
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
        result << bucket->slots[j].value();
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
        size_t hash = get_hasher_ref()(buckets_[i].slots[j].value());
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

// TODO: This `value` argument shouldn't be `const value_type &`, it
// should just be `value_type`, but there is a bug in the map code
// that makes that fail.

// TODO: It looks like we lost the bubble.

template <class Traits>
template <bool insert_tombstones>
void HashTable<Traits>::InsertAscending(value_type value,
                                        size_t &first_uninitialized_bucket) {
  const key_type &key = Traits::KeyOf(value);
  const size_t hash = get_hasher_ref()(key);
  const size_t preferred_bucket = buckets_.H1(hash);
  const size_t h2 = buckets_.H2(hash);
  size_t bucket_to_try = preferred_bucket;
  while (true) {
    // TODO: Do something better if this CHECK fails.
    CHECK_LT(bucket_to_try, buckets_.physical_size());
    assert(bucket_to_try < buckets_.physical_size());
    Bucket<Traits> &bucket = buckets_[bucket_to_try];
    while (bucket_to_try >= first_uninitialized_bucket) {
      buckets_[first_uninitialized_bucket].Init();
      ++first_uninitialized_bucket;
    }
    size_t matches = bucket.FindEmpties();
    if constexpr (insert_tombstones && Traits::kTombstonePeriod.has_value()) {
      size_t global_slot_number = bucket_to_try * Traits::kSlotsPerBucket;
      size_t mod_period = global_slot_number % *Traits::kTombstonePeriod;
      static_assert(Traits::kSlotsPerBucket < *Traits::kTombstonePeriod);
      if (mod_period >= *Traits::kTombstonePeriod - Traits::kSlotsPerBucket) {
        // Leave a tombstone in the last bucket of the ones in the
        // same period.
        // Assert that we haven't filled the tombstone already.
        assert(matches % 2 == 1);
        // remove one possible empty tombstone from the empty list.
        matches -= 1;
      }
    }
    if (matches != 0) {
      size_t idx = CountTrailingZeros(matches);
      assert(bucket.h2[idx] == Traits::kEmpty);
      bucket.h2[idx] = h2;
      assert(h2 < Traits::kH2Modulo);
      new (&bucket.slots[idx].value()) value_type(std::move(value));
      maxf(buckets_[preferred_bucket].search_distance,
           bucket_to_try - preferred_bucket + 1);
      // TODO: Factor out the ++size_.
      return;
    }
    ++bucket_to_try;
  }
}

template <class Traits>
void HashTable<Traits>::FinishInsertAscending(
    size_t first_uninitialized_bucket) {
  while (first_uninitialized_bucket < buckets_.physical_size()) {
    buckets_[first_uninitialized_bucket].Init();
    ++first_uninitialized_bucket;
  }
  // Set the end-of-search sentinal.
  buckets_[first_uninitialized_bucket - 1].search_distance =
      Traits::kSearchDistanceEndSentinal;
}

template <class Traits>
void HashTable<Traits>::CopyFrom(const Buckets<Traits> &buckets) {
  size_t bucket_number = 0;
  size_t first_uninitialized_bucket = 0;
  for (const Bucket<Traits> &bucket : buckets) {
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != Traits::kEmpty) {
        // TODO: We could save recomputing h2 possibly.
        InsertAscending</*insert_tombstones=*/false>(bucket.slots[j].value(), first_uninitialized_bucket);
      }
    }
    ++bucket_number;
  }
  FinishInsertAscending(first_uninitialized_bucket);
}

template <class Traits>
void HashTable<Traits>::RehashFrom(Buckets<Traits> &buckets) {
  size_t bucket_number = 0;
  size_t first_uninitialized_bucket = 0;
  for (Bucket<Traits> &bucket : buckets) {
    // Periodically release the memory for the buckets that we no longer need.
    if (bucket_number % (1ul << 15) == 0) {
      uintptr_t start = reinterpret_cast<uintptr_t>(buckets.begin());
      uintptr_t here = reinterpret_cast<uintptr_t>(&bucket);
      uintptr_t start_rounded_up = (start + 4095) / 4096 * 4096;
      uintptr_t here_rounded_down = here / 4096 * 4096;
      if (start_rounded_up < here_rounded_down) {
        uintptr_t len = here_rounded_down - start_rounded_up;
        if (madvise(reinterpret_cast<void *>(start_rounded_up), len,
                    MADV_DONTNEED)) {
          perror("DONTNEED failed");
        }
      }
    }
    for (size_t j = 0; j < Traits::kSlotsPerBucket; ++j) {
      if (bucket.h2[j] != Traits::kEmpty) {
        // TODO: We could save recomputing h2 possibly.
        //
        // TODO: When value_type is `pair<const K, V>`, this std::move
        // doesn't have any effect, resulting in a copy.  We'd like to
        // have the key type be a `pair<K, V>` that we cast to
        // `pair<const K, V>&`.  That can only be done if the pair has
        // standard layout and the offsets of `K` and `V`.
        InsertAscending</*insert_tombstones*/true>(std::move(bucket.slots[j].value()),
                                                   first_uninitialized_bucket);
        // Destroy the value so that we can destroy the bucket without
        // running a bunch of destructors.
        bucket.slots[j].value().~value_type();
      }
    }
    ++bucket_number;
  }
  FinishInsertAscending(first_uninitialized_bucket);
  buckets.Deallocate();
}

template <class Traits> void HashTable<Traits>::rehash(size_t slot_count) {
  if (slot_count == 0) {
    slot_count = ceil(size() * Traits::full_utilization_denominator,
                      Traits::full_utilization_numerator);
  }
  Buckets<Traits> buckets(ceil(slot_count, Traits::kSlotsPerBucket));
  buckets.swap(buckets_);
  // Leaves size_ unmodified.
  RehashFrom(buckets);
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
      if (bucket.h2[j] != Traits::kEmpty && bucket.slots[j].value() == value) {
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
