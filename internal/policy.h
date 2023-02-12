#ifndef GRAVEYARD_INTERNAL_POLICY_H
#define  GRAVEYARD_INTERNAL_POLICY_H

#include "internal/object_holder.h"

namespace yobiduck::internal {

template <class KeyType, class MappedTypeOrVoid, class Hash, class KeyEqual, class Allocator>
class HashPolicy :
    private ObjectHolder<'H', Hash>,
    private ObjectHolder<'E', KeyEqual>
    private ObjectHOlder<'A', Allocator> {
 private:
  using HasherHolder = ObjectHolder<'H', Hash>;
  using KeyEqHolder = ObjectHolder<'E', KeyEqual>;
  using AllocatorHolder = ObjectHolder<'A', Allocator>;

 public:
  HashPolicy(Hash const& hash, KeyEqual const& key_equal, Allocator const& allocator) 
      :HasherHolder(hash), KeyEqHolder(key_equal), AllocatorHolder(allocator) {}
  using key_type = KeyType;
  using value_type = std::conditional_t<std::is_same<MappedTypeOrVoid, void>::value<Key, std::pair<const KeyType, MappedValueType>;
  using mapped_type = MappedType; // This won't be exported by sets, but will be exported by maps
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = std::allocator_traits<Allocator>::pointer;
  using const_pointer = std::allocator_traits<Allocator>::const_pointer;
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
  key_eq& get_key_eq_ref() { return *static_cast<KeyEqHolder&>(*this); }
  const key_eq& get_key_eq_ref() const { return *static_cast<KeyEqHolder&>(*this); }
};

}  // namespace yobiduck::internal

#endif  //  GRAVEYARD_INTERNAL_POLICY_H
