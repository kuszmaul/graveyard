#include "tombstone_set.h"

#include "absl/log/check.h"

int main() {
  yobiduck::TombstoneSet<uint64_t> set;
  CHECK_EQ(set.size(), 0ul);
  CHECK(set.insert(0ul));
}
