#ifndef _GRAVEYARD_INTERNAL_HASH_TABLE_H
#define _GRAVEYARD_INTERNAL_HASH_TABLE_H

#include "internal/object_holder.h"

namespace yobiduck::internal {

template <class KeyType, class MappedTypeOrVoid, class Hash, class KeyEqual, class Allocator>
struct HashTableTraits {
  using key_type = KeyType;
  using mapped_type_or_void = MappedTypeOrVoid;  
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator = Allocator;
};

template <class Traits>
class HashTable :
    private ObjectHolder<'H', typename Traits::hasher>,
    private ObjectHolder<'E', typename Traits::key_equal>,
    private ObjectHolder<'A', typename Traits::allocator> {
 private:
  using HasherHolder = ObjectHolder<'H', typename Traits::hasher>;
  using KeyEqualHolder = ObjectHolder<'E', typename Traits::key_equal>;
  using AllocatorHolder = ObjectHolder<'A', typename Traits::allocator>;

 public:
  using key_type = typename Traits::key_type;
  using value_type = typename std::conditional<std::is_same<typename Traits::mapped_type_or_void, void>::value,
                                               key_type,
                                               std::pair<const key_type, typename Traits::mapped_type_or_void>>::type;
  // This won't be exported by sets, but will be exported by maps.
  using mapped_type = typename Traits::mapped_type_or_void;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = typename Traits::hasher;
  using key_equal = typename Traits::key_equal;
  using allocator_type = typename Traits::allocator;

  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
  HashTable() :HashTable(0, hasher(), key_equal(), allocator_type()) {}
  explicit HashTable(size_t initial_capacity, hasher const& hash = hasher(), key_equal const& key_eq = key_equal(), allocator_type const& allocator = allocator_type()) 
      :HasherHolder(hash), KeyEqualHolder(key_eq), AllocatorHolder(allocator) {
    reserve(initial_capacity);
  }
  // Copy constructor
  explicit HashTable(const HashTable& other) :HashTable(other, std::allocator_traits<allocator_type>::select_on_container_copy_construction(get_allocator_ref())) {}
  HashTable(const HashTable& other, const allocator_type& a) :HashTable(other.size(), other.get_hasher_ref(), other.get_key_eq_ref(), a) {
    for (const auto& v: other) {
      insert(v); // TODO: Take advantage of the fact that we know `v` isn't in `*this`.
    }
  }

  // iterator not here
  // const_iterator not here
  // node_type not here
  // insert_return_type not here
  allocator_type get_allocator() const { return get_allocator_ref(); }
  allocator_type& get_allocator_ref() { return *static_cast<AllocatorHolder&>(*this); }
  const allocator_type& get_allocator_ref() const { return *static_cast<AllocatorHolder&>(*this); }
  hasher hash_function() const { return get_hasher_ref(); }
  hasher& get_hasher_ref() { return *static_cast<HasherHolder&>(*this); }
  const hasher& get_hasher_ref() const { return *static_cast<HasherHolder&>(*this); }
  key_equal key_eq() const { return get_key_eq_ref(); }
  key_equal& get_key_eq_ref() { return *static_cast<KeyEqualHolder&>(*this); }
  const key_equal& get_key_eq_ref() const { return *static_cast<KeyEqualHolder&>(*this); }

  void reserve(size_t count);
};

}  // namespace yobiduck::internal

#endif  // _GRAVEYARD_INTERNAL_HASH_TABLE_H
