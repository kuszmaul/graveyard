#include "hash_benchmark.h"

#include <random>
#include <set>
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
  if (it != results_.end()) {
    std::cerr << "Reinserting " << implementation << " " << operation << " " << size << std::endl;
  }
  assert(it == results_.end());
  results_.insert({std::move(key), std::move(result)});
}

void HashBenchmarkResults::Print() const {
  std::cout << "|Implementation|Operation|Table size|Time/op|Memory Utilization|" << std::endl;
  std::cout << "|--------------|---------|---------:|------:|-----------------:|" << std::endl;
  for (const auto& [key, result] : results_) {
    std::cout << "|`" << key.implementation << "`|" << key.operation << "|" << key.input_size << "|" << absl::StrFormat("%.1f", result.Mean()) << "±" << absl::StrFormat("%.1f", result.StandardDeviation() * 2) << "ns|" << absl::StrFormat("%+.1f", result.MinimalMemoryEstimate() * 100.0 / result.MemorySize()) << "%|" << std::endl;
  }
}

namespace {
constexpr std::string_view kReferenceImplementation = "flat_hash_set";
const std::vector<std::string_view> kOtherImplementations = {"SimpleIntegerLinearProbing"};
const std::vector<std::string_view> kOps = {"insert"};

template<class Table, class Key>
const typename Table::value_type& FindOrDie(const Table& table, const Key& key) {
  auto it = table.find(key );
  if (it == table.end()) {
    abort();
  }
  return *it;
}

std::string TimeString(const BenchmarkResult &result) {
  return absl::StrFormat("%.1f±%.1fns", result.Mean(), result.StandardDeviation() * 2);
}
}

void HashBenchmarkResults::Print2() const {
  for (std::string_view op : kOps) {
    PrintOp(op);
  }
}

void HashBenchmarkResults::PrintOp(std::string_view op) const {
  std::set<size_t> sizes;
  for (const auto& [key, result] : results_) {
    if (key.operation == op) {
      sizes.insert(key.input_size);
    }
  }
  std::cout << "## " << op << std::endl;
  std::cout << "|Size|Time/op|Memory|";
  for (auto other : kOtherImplementations) {
    std::cout << other << " Time/op|Time Change|";
  }
  std::cout << "Memory|Memory Change|";
  std::cout << std::endl;
  std::cout << "|---:|------:|";
  for (auto other [[maybe_unused]] : kOtherImplementations) {
    std::cout << "---:|---:|---:|---:|---:|";
  }
  std::cout << std::endl;
  for (size_t size : sizes) {
    const auto& [ref_key, ref_result] = FindOrDie(results_, Key{std::string(kReferenceImplementation), std::string(op), size});
    std::cout << "|" << ref_key.input_size << "|" << TimeString(ref_result) << "|" << absl::StrFormat("%.1f%%", ref_result.MinimalMemoryEstimate() * 100.0 / ref_result.MemorySize()) <<
        "|";
    double ref_memory_size = ref_result.MemorySize();
    for (auto other : kOtherImplementations) {
      const auto& [other_key, other_result] = FindOrDie(results_, Key{std::string(other), std::string(op), size});
      double other_memory_size = other_result.MemorySize();
      std::cout << TimeString(other_result) << "|" << absl::StrFormat("%+.1f%%", 100.0 * (other_result.Mean() - ref_result.Mean()) / ref_result.Mean()) << "|";
      std::cout << absl::StrFormat("%.1f%%", other_result.MinimalMemoryEstimate() * 100.0 / other_memory_size) << "|";
      if (ref_result.MinimalMemoryEstimate() != other_result.MinimalMemoryEstimate()) abort();
      std::cout << absl::StrFormat("%+.1f%%", 100.0 * (other_memory_size - ref_memory_size) / ref_memory_size) << "|";
    }
    std::cout << std::endl;
  }
}
#if 0
  std::string_view kSimple = "SimpleIntegerLinearProbing";
  std::string_view kFlat = "flat_hash_set";
  std::string_view kInsert = "insert";
  std::set<size_t> sizes;
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
