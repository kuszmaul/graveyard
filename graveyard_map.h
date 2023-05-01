#ifndef _GRAVEYARD_MAP_H_
#define _GRAVEYARD_MAP_H_

// IWYU has some strange behavior around std::pair.  It wants to get rid
// of utility and add iterator.

#include <memory>  // for allocator
#include <utility> // IWYU pragma: keep

#include "absl/container/flat_hash_set.h" // For hash_default_hash (TODO: use internal/hash_function_defaults.h
#include "internal/hash_table.h"

// IWYU pragma: no_include <iterator>

namespace yobiduck {

// Hash Map with Graveyard hashing.
template <class Key, class T,
          class Hash = absl::container_internal::hash_default_hash<Key>,
          class KeyEqual = absl::container_internal::hash_default_eq<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
class GraveyardMap
    : private yobiduck::internal::HashTable<yobiduck::internal::HashTableTraits<
          Key, T, Hash, KeyEqual, Allocator>> {
  using Traits =
      yobiduck::internal::HashTableTraits<Key, T, Hash, KeyEqual, Allocator>;
  using Base = yobiduck::internal::HashTable<Traits>;

  template <class K> using key_arg = typename Traits::template key_arg<K>;

public:
  // The order as found in
  // https://en.cppreference.com/w/cpp/container/unordered_set.  Put blank lines
  // between items to prevent clang_format from reordering them.

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

  // Default constructor
  //  GraveyardMap();
  //
  // Copy constructor
  //  GraveyardMap(const GraveyardSet &set);
  using Base::Base;

  // Copy assignment
  using Base::operator=;

  using Base::clear;

  using Base::swap;

  using typename Base::iterator;

  using typename Base::const_iterator;

  using Base::begin;

  using Base::cbegin;

  using Base::end;

  using Base::cend;

  using Base::insert;

  using Base::emplace;

  using Base::try_emplace;

  template <class K = key_type> T &operator[](const key_arg<K> &key) {
    auto [it, inserted] = try_emplace(key);
    return it->second;
  }

  using Base::count;

  using Base::find;

  using Base::contains;

  using Base::equal_range;

  using Base::empty;

  using Base::size;

  // size_t capacity() const;
  //
  // Effect: Returns the number of element slots available in *this.
  //
  // Note: Not part of the `std::unordered_set` API.
  using Base::capacity;

  // size_t bucket_count() const;
  //
  // Effect: Returns capacity().
  using Base::bucket_count;

  using Base::rehash;

  using Base::reserve;

  // size_t GetAllocatedMemorySize() const;
  //
  // Effect: Returns the amount of memory allocated in *this.  Doesn't include
  // the `*this`.
  //
  // Note: Not part of the `stl::unordered_set` API.
  using Base::GetAllocatedMemorySize;

  using Base::GetProbeStatistics;
  using Base::GetSuccessfulProbeLength;

  using Base::GetSortedBucketsIterator;
};

} // namespace yobiduck

#endif // _GRAVEYARD_MAP_H_
