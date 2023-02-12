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
  HashTable() :HashTable(hasher(), key_equal(), allocator_type()) {}
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
 private:
  HashTable(Hash const& hash, KeyEqual const& key_equal, Allocator const& allocator) 
      :HasherHolder(hash), KeyEqHolder(key_equal), AllocatorHolder(allocator) {}

};

}  // namespace yobiduck::internal

#endif  // _GRAVEYARD_INTERNAL_HASH_TABLE_H
