#include "hash_benchmark.h"

#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>  // for pair
#include <vector>

#include "contains.h"
#include "absl/strings/str_format.h"

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


void HashBenchmarkResults::Add(std::string_view implementation, std::string_view operation,
                               size_t size, BenchmarkResult result) {
  Key key = {.implementation = std::string(implementation),
      .operation = std::string(operation),
      .input_size = size};
  auto it [[maybe_unused]] = results_.find(key);
  assert(it == result_.end());
  results_.insert({std::move(key), std::move(result)});
}

void HashBenchmarkResults::Print() const {
  std::cout << "|Implementation|Operation|Table size|Time/op|Memory Utilization|" << std::endl;
  std::cout << "|--------------|---------|---------:|------:|-----------------:|" << std::endl;
  for (const auto& [key, result] : results_) {
    std::cout << "|`" << key.implementation << "`|" << key.operation << "|" << key.input_size << "|" << absl::StrFormat("%.1f", result.Mean()) << "±" << absl::StrFormat("%.1f", result.StandardDeviation() * 2) << "ns|" << absl::StrFormat("%.1f", result.MinimalMemoryEstimate() * 100.0 / result.MemorySize()) << "%|" << std::endl;
  }
}

namespace {
constexpr std::string_view reference_implementation = "flat_hash_set";
constexpr std::vector<std::string_view> other_implementations = {"SimpleIntegerLinearProbing"};
constexpr std::vector<std::string_view> ops = {"insert"};
}

void HashBenchmarkResults::Print2() const {
  for (std::string_view op : ops) {
  }
}
#if 0
  std::string_view kSimple = "SimpleIntegerLinearProbing";
  std::string_view kFlat = "flat_hash_set";
  std::string_view kInsert = "insert";
  std::set<size_t> sizes;
  for (const auto& [key, result] : results_) {
    if ((key.implementation==kSimple || key.implementation == kFlat)
        && key.operation == kInsert) {
      sizes.insert(key.size);
    }
  }
  for (size_t size : sizes) {
    auto flat = results_.find(Key{kFlat, kInsert, size});
    assert(flat != results_.end());

    auto simple = results_.find(Key{kSimple, kInsert, size});
    assert(simple != results_.end());

    std::cout << "|" << size << "|" << PrintTime(*flat) << "|" << PrintTime(*simple) << "|" <<
  }
}
#endif

bool operator<(const HashBenchmarkResults::Key& a, const HashBenchmarkResults::Key& b) {
  if (a.operation < b.operation) return true;
  if (b.operation < a.operation) return false;
  if (a.input_size < b.input_size) return true;
  if (b.input_size < a.input_size) return false;
  if (a.implementation < b.implementation) return true;
  if (b.implementation < a.implementation) return false;
  return false;
}
