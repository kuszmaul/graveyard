#ifndef CONTAINS_H_
#define CONTAINS_H_

// C++17 `std::unordered_set` doesn't have contains.  So we implement it here for
// any container that has `find` and `end`.
template<class Container, class Value>
bool Contains(const Container& container, const Value& value) {
  return container.find(value) != container.end();
}

#endif  // CONTAINS_H_
