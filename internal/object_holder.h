#ifndef _GRAVEYARD_INTERNAL_OBJECT_HOLDER_H_
#define _GRAVEYARD_INTERNAL_OBJECT_HOLDER_H_

// Implement an object holder which is intended to be used as a base class.
// This allows us to avoid allocating space for an empty object (such as Hash
// and Eq (which are empty in the common case) in a hash table).
//
// This idea comes from F14.  In contrast, Abseil uses a fancy templated
// `Layout` class to store the empty objects.  The F14 scheme seems simpler.

#include <utility>

namespace yobiduck::internal {

template <char Tag, class T>
class ObjectHolder {
 public:
  template <typename... Args>
  ObjectHolder(Args&&... args) : value_{std::forward<Args>(args)...} {}

  T& operator*() { return value_; };
  const T& operator*() const { return value_; }

 private:
  T value_;
};

}  // namespace yobiduck::internal

#endif  // _GRAVEYARD_INTERNAL_OBJECT_HOLDER_H_
