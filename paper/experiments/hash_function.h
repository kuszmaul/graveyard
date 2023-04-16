#ifndef HASH_FUNCTION_H_
#define HASH_FUNCTION_H_

#include <cstddef>
#include <cstdint>

static constexpr size_t H1(uint64_t v, size_t size) {
  return size_t((__int128(v) * __int128(size)) >> 64);
}

#endif
