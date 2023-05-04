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
  template <class Pair1, class Pair2>
  static constexpr bool LayoutCompatible() {
    // Some compilers require us to write this rather than just having
    // a constexpr conjunction: They warn about offsetof being
    // conditionally supported.  By using `if constexpr` we avoid that
    // warning.
    if constexpr (std::is_standard_layout_v<Pair1> &&
                  std::is_standard_layout_v<Pair2>) {
      return
          sizeof(Pair1) == sizeof(Pair2) &&
          alignof(Pair1) == alignof(Pair2) &&
          offsetof(Pair1, first) == offsetof(Pair2, first) &&
          offsetof(Pair1, second) == offsetof(Pair1, second);
    } else {
      return false;
    }
  }

  static constexpr bool is_overlayable = LayoutCompatible<MutablePair, ConstPair>();

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
    new (&u_.stored) StoredType(std::move(value));
  }

  // Return a reference to the ConstPair.  If it's overlayable, then
  // we have to do a cast to convert the MutablePair to a ConstPair.
  // Otherwise MutablePair and ConstPair are the same.
  //
  // The returned reference isn't `const`, since it can be used to
  // modify `second`.  But `first` is const.
  const VisibleType& GetValue() const {
    // If it's overlayable, this reinterpret cast actually does something.
    if constexpr (std::is_same_v<VisibleType, StoredType>) {
      // If the types are the same, then avoid calling std::launder,
      // which inhibits certain optimizations.
      return u_.stored;
    } else {
      return *std::launder(reinterpret_cast<const VisibleType*>(&u_.stored));
    }
  }
  VisibleType& GetValue() {
    if constexpr (std::is_same_v<VisibleType, StoredType>) {
      return u_.stored;
    } else {
      return *std::launder(reinterpret_cast<VisibleType*>(&u_.stored));
    }
  }
  StoredType MoveAndDestroy() {
    StoredType result = std::move(u_.stored);
    Destroy();
    return result;
  }
  void Destroy() {
    u_.stored.~StoredType();
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
