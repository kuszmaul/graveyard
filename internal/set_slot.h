#ifndef _GRAVEYARD_INTERNAL_SET_SLOT_H_
#define _GRAVEYARD_INTERNAL_SET_SLOT_H_

#include <utility>

namespace yobiduck::internal {

// See map_slot.h for the corresponding structure for unordered maps.

template<class Value>
class SetSlot {
 public:
  using StoredType = Value;
  using VisibleType = Value;
  void Store(StoredType value) {
    new (&u_.value) StoredType(std::move(value));
  }
  void Transfer(SetSlot &from) {
    new (&u_.value) StoredType(std::move(from.u_.value));
    from.Destroy();
  }
  const VisibleType& GetValue() const {
    return u_.value;
  }
  StoredType MoveAndDestroy() {
    StoredType result = std::move(u_.value);
    Destroy();
    return result;
  }
  void Destroy() {
    u_.value.~StoredType();
  }
 private:
  union {
    char bytes_[sizeof(StoredType)];
    StoredType value;
  } u_;
};

}  // namespace yobiduck::internal

#endif // _GRAVEYARD_INTERNAL_SET_SLOT_H_
