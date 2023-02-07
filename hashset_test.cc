#include <unordered_set>

#include "absl/log/check.h"
#include "hash_span.h"

int main() {
  SetSpan<uint64_t> uset = SetSpan<uint64_t>(std::unordered_set<uint64_t>());
  CHECK_EQ(uset.size(), 0ul);
}
