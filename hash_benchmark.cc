#include "hash_benchmark.h"

#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>        // for pair
#include <vector>

#include "contains.h"

namespace {
// `numbers[n]` contains a set of cardinality `n`.
std::unordered_map<size_t, std::vector<uint64_t>> numbers;
// `other_numbers[n]` contains a set of cardinality `n` and doesn't intersect
// `numbers[i]`.
std::unordered_map<size_t, std::vector<uint64_t>> other_numbers;
std::random_device r;
std::default_random_engine e1(r());
std::uniform_int_distribution<uint64_t> uniform_dist;
}  // namespace

const std::vector<uint64_t> GetSomeNumbers(size_t size) {
  {
    auto it = numbers.find(size);
    if (it != numbers.end()) {
      return it->second;
    }
  }
  std::unordered_set<uint64_t> values;
  while (values.size() < size) {
    values.insert(uniform_dist(e1));
  }
  return numbers[size] = std::vector(values.begin(), values.end());
}

const std::vector<uint64_t> GetSomeOtherNumbers(size_t size) {
  {
    auto it = other_numbers.find(size);
    if (it != other_numbers.end()) {
      return it->second;
    }
  }
  std::unordered_set<uint64_t> values;
  while (values.size() < size) {
    uint64_t value = uniform_dist(e1);
    if (!Contains(numbers, value)) {
      values.insert(value);
    }
  }
  return numbers[size] = std::vector(values.begin(), values.end());
}
