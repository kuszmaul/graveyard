#ifndef _GRAVEYARD_INTERNAL_HASH_TABLE_H
#define _GRAVEYARD_INTERNAL_HASH_TABLE_H

#include "internal/object_holder.h"

namespace yobiduck::internal {

template <class KeyType, class MappedTypeOrVoid, class Hash, class KeyEqual, class Allocator>
class HashTable :
    private ObjectHolder<'H', Hash>,
    private ObjectHolder<'E', KeyEqual>,
    private ObjectHolder<'A', Allocator> {
 private:
  using HasherHolder = ObjectHolder<'H', Hash>;
  using KeyEqHolder = ObjectHolder<'E', KeyEqual>;
  using AllocatorHolder = ObjectHolder<'A', Allocator>;

 public:
  using key_type = KeyType;
  using value_type = typename std::conditional<std::is_same<MappedTypeOrVoid, void>::value,
                                     KeyType,
                                     std::pair<const KeyType, MappedTypeOrVoid>>::type;
  // This won't be exported by sets, but will be exported by maps.
  using mapped_type = MappedTypeOrVoid;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator_type = Allocator;

  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
  HashTable() :HashTable(0, hasher(), key_equal(), allocator_type()) {}
  explicit HashTable(size_t initial_capacity, Hash const& hash = hasher(), KeyEqual const& key_eq = key_equal(), Allocator const& allocator = allocator_type()) 
      :HasherHolder(hash), KeyEqHolder(key_eq), AllocatorHolder(allocator) {
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
  key_equal& get_key_eq_ref() { return *static_cast<KeyEqHolder&>(*this); }
  const key_equal& get_key_eq_ref() const { return *static_cast<KeyEqHolder&>(*this); }

  void reserve(size_t count);
};

}  // namespace yobiduck::internal

#endif  // _GRAVEYARD_INTERNAL_HASH_TABLE_H
