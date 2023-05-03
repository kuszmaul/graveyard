#ifndef _GRAVEYARD_INTERNAL_MAP_SLOT_H_
#define _GRAVEYARD_INTERNAL_MAP_SLOT_H_

#include <type_traits>
#include <utility>


namespace yobiduck::internal {

// This is hackery to allow us to convert a `pair<const K, V>&` to a
// `pair<K, V>&` (when possible) so that we can avoid constructing a
// new key.
//
// See set_slot.h for the corresponding structure for unordered sets.

// MapSlot is trivial.
template<class Key, class Mapped>
class MapSlot {
  using key_type = Key;
  using mapped_type = Mapped;
  // Really it is the key that is mutable or const.
  using MutablePair = std::pair<key_type, mapped_type>;
  using ConstPair = std::pair<const key_type, mapped_type>;
  static constexpr bool is_overlayable =
      std::is_standard_layout_v<MutablePair> &&
      std::is_standard_layout_v<ConstPair> &&
      sizeof(MutablePair) == sizeof(ConstPair) &&
      alignof(MutablePair) == alignof(ConstPair) &&
      offsetof(MutablePair, first) == offsetof(ConstPair, first) &&
      offsetof(MutablePair, second) == offsetof(MutablePair, second);
  // TODO: Think about when we need to `std::launder` the memory.

 public:
  using StoredType = std::conditional_t<is_overlayable, MutablePair, ConstPair>;
  using VisibleType = ConstPair;

  // We always store into the MaybeMutablePair.
  //
  // When we need to export a ConstPair to the user, we do a cast from
  // the MaybeMutablePair to the ConstPair.  (The types are the same
  // if the types cannot be overlayed properly).
  //

  void Store(StoredType value) {
    new (~u_.stored) StoredType(std::move(value));
  }

  // Return a reference to the ConstPair.  If it's overlayable, then
  // we have to do a cast to convert the MutablePair to a ConstPair.
  // Otherwise MutablePair and ConstPair are the same.
  const VisibleType& GetValue() const {
    // If it's overlayable, this reinterpret cast actually does something.
    return reinterpret_cast<VisibleType&>(u_.stored);
  }
  StoredType MoveAndDestroy() {
    StoredType result = std::move(u_.stored);
    u_.stored.~StoredType();
    return result;
  }

 private:
  union {
    char bytes[sizeof(StoredType)];
    StoredType stored;
    VisibleType visible;
  } u_;
};

} // namespace yobiduck::internal

#endif // _GRAVEYARD_INTERNAL_MAP_SLOT_H_
