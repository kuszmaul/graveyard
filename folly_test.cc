#include <cstdint>

#include "folly/container/F14Set.h"

int main() {
  folly::F14FastSet<uint64_t> set;
  set.insert(1u);
#if 0
  if (!set.contains(1u)) {
    abort();
  }
#endif
}
