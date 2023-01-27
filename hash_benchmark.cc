#include "hash_benchmark.h"

#include <cstdlib>  // for abort
#include <random>
#include <vector>

#include "absl/container/flat_hash_set.h"  // for flat_hash_set, BitMask
#include "absl/hash/hash.h"                // for Hash

namespace {
std::random_device r;
std::default_random_engine e1(r());
std::uniform_int_distribution<uint64_t> uniform_dist;
}  // namespace

void GetSomeNumbers(size_t size, std::vector<uint64_t> &result) {
  result.clear();
  result.reserve(size);
  absl::flat_hash_set<uint64_t> values;
  while (values.size() < size) {
    uint64_t value = uniform_dist(e1);
    auto [it, inserted] = values.insert(value);
    if (inserted) {
      result.push_back(value);
    }
  }
}

std::vector<uint64_t> GetSomeOtherNumbers(
    const std::vector<uint64_t>& other_numbers) {
  absl::flat_hash_set<uint64_t> other(other_numbers.begin(),
                                      other_numbers.end());
  absl::flat_hash_set<uint64_t> values;
  std::vector<uint64_t> vec;
  while (values.size() < other_numbers.size()) {
    uint64_t value = uniform_dist(e1);
    if (!other.contains(value)) {
      auto [it, inserted] = values.insert(value);
      if (inserted) {
        vec.push_back(value);
      }
    }
  }
  return vec;
}

namespace {
constexpr std::string_view kReferenceImplementation = "flatset";
const std::vector<std::string_view> kOtherImplementations = {"SimpleILP",
                                                             "flatset-nohash"};
const std::vector<std::string_view> kOps = {
    "insert", "reserved-insert", "contains-found", "contains-not-found"};

template <class Table, class Key>
const typename Table::value_type& FindOrDie(const Table& table,
                                            const Key& key) {
  auto it = table.find(key);
  if (it == table.end()) {
    abort();
  }
  return *it;
}
}  // namespace
