#ifndef _GRAVEYARD_SET_H_
#define _GRAVEYARD_SET_H_

#include <malloc.h>

#include <algorithm>
#include <cassert>
#include <cstddef>  // for size_t
#include <cstdint>  // for uint64_t
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"  // For hash_default_hash (TODO: use internal/hash_function_defaults.h
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "internal/hash_table.h"

namespace yobiduck {

// Hash Set with Graveyard hashing.  Type `T` must have a default constructor.
// (TODO: Remove that restrictionon the default constructor).
template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class KeyEqual = absl::container_internal::hash_default_eq<T>,
          class Allocator = std::allocator<T>>
class GraveyardSet
    : private yobiduck::internal::HashTable<yobiduck::internal::HashTableTraits<
          T, void, Hash, KeyEqual, Allocator>> {
  using Traits =
      yobiduck::internal::HashTableTraits<T, void, Hash, KeyEqual, Allocator>;
  using Base = yobiduck::internal::HashTable<Traits>;

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

  GraveyardSet() = default;

  // Copy constructor
  //  GraveyardSet(const GraveyardSet &set);
  using Base::Base;

  // Copy assignment
  GraveyardSet& operator=(const GraveyardSet& other) {
    clear();
    reserve(other.size());
    for (const T& value : other) {
      insert(value);  // TODO: Optimize this given that we know `value` is not
                      // in `*this`.
    }
  }

  // Move constructor
  GraveyardSet(GraveyardSet&& other) : GraveyardSet() { swap(other); }

  // Move assignment
  GraveyardSet& operator=(GraveyardSet&& other) {
    swap(other);
    return *this;
  }

  using Base::clear;

  using Base::swap;

  using Base::iterator;

  using Base::const_iterator;

  using Base::begin;

  using Base::cbegin;

  using Base::end;

  using Base::cend;

  using Base::insert;

  using Base::contains;

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
};

// TODO: Idea, keep a bit that says whether a particular slot is out of its
// preferred bucket.  Then during insert we can avoid computing most of the
// hashes.

}  // namespace yobiduck

#endif  // _GRAVEYARD_SET_H_
