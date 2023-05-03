#ifndef _GRAVEYARD_INTERNAL_HASH_MAP_H
#define _GRAVEYARD_INTERNAL_HASH_MAP_H

#include "internal/hash_table.h"

namespace yobiduck::internal {

template <class Traits>
class HashMap : public HashTable<Traits> {

 private:
  using Base = HashTable<Traits>;

  template <class K> using key_arg = typename Base::template key_arg<K>;

 public:
  using typename Base::key_type;
  using typename Base::value_type;
  using typename Base::const_iterator;
  using typename Base::iterator;

  // Overload:
  //   pair<iterator, bool> try_emplace(key_type&& k, Args&&...);
  template <class K = key_type, class... Args,
            typename std::enable_if_t<
              !std::is_convertible_v<K, const_iterator>, int> = 0,
            K* = nullptr>
  std::pair<iterator, bool>
  try_emplace(key_arg<K>&& key, Args&&...args) {
    // TODO: It looks like a bug here.  Shouldn't this be
    //  std::forward<key_arg<K>>(k)
    // This bug, if it is a bug, is from absl raw_hash_map.h line 125.
    return TryEmplace(std::forward<K>(key), std::forward<Args>(args)...);
  }

  // Overload:
  //   pair<iterator, bool> try_emplace(key_type& k, Args&&...);
  template <class K = key_type, class... Args,
            typename std::enable_if_t<
              !std::is_convertible_v<K, const_iterator>, int> = 0,
            K* = nullptr>
  std::pair<iterator, bool>
  try_emplace(const key_arg<K>& key, Args &&...args) {
    return TryEmplace(key, std::forward<Args>(args)...);
  }

  // Overload:
  //   iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args)
  // Just ignores the hint
  template <class K = key_type, class... Args, K* = nullptr>
  iterator try_emplace(const_iterator, key_arg<K>&& k, Args&&... args) {
    return try_emplace(std::forward<K>(k), std::forward<Args>(args)...).first;
  }

  // Overload:
  //   iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args)
  // Just ignores the hint
  template <class K = key_type, class... Args>
  iterator try_emplace(const_iterator, const key_arg<K>& k, Args&&... args) {
    return try_emplace(k, std::forward<Args>(args)...).first;
  }

 private:
  using Base::PrepareInsert;

  template <class K = key_type, class... Args>
  std::pair<iterator, bool> TryEmplace(K&& key, Args &&...args) {
    auto prepare_result = PrepareInsert(key);
    auto &[it, inserted] = prepare_result;
    if (inserted) {
      new (&*it) value_type(std::piecewise_construct,
                            std::forward_as_tuple(std::forward<K>(key)),
                            std::forward_as_tuple(std::forward<Args>(args)...));
    }
    return prepare_result;
  }

};

}  // namespace yobiduck::internal

#endif // _GRAVEYARD_INTERNAL_HASH_MAP_H
