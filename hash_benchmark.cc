#include "hash_benchmark.h"

#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>  // for pair
#include <vector>

#include "absl/strings/str_format.h"
#include "contains.h"

namespace {
std::random_device r;
std::default_random_engine e1(r());
std::uniform_int_distribution<uint64_t> uniform_dist;
}  // namespace

std::vector<uint64_t> GetSomeNumbers(size_t size) {
  std::vector<uint64_t> vec;
  absl::flat_hash_set<uint64_t> values;
  while (values.size() < size) {
    uint64_t value = uniform_dist(e1);
    auto [it, inserted] = values.insert(value);
    if (inserted) {
      vec.push_back(value);
    }
  }
  return vec;
}

std::vector<uint64_t> GetSomeOtherNumbers(const std::vector<uint64_t>& other_numbers) {
  absl::flat_hash_set<uint64_t> other(other_numbers.begin(), other_numbers.end());
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

void HashBenchmarkResults::Add(std::string_view implementation,
                               std::string_view operation, size_t size,
                               BenchmarkResult result) {
  Key key = {.implementation = std::string(implementation),
             .operation = std::string(operation),
             .input_size = size};
  auto it [[maybe_unused]] = results_.find(key);
  if (it != results_.end()) {
    std::cerr << "Reinserting " << implementation << " " << operation << " "
              << size << std::endl;
  }
  assert(it == results_.end());
  results_.insert({std::move(key), std::move(result)});
}

void HashBenchmarkResults::Print() const {
  std::cout
      << "|Implementation|Operation|Table size|Time/op|Memory Utilization|"
      << std::endl;
  std::cout
      << "|--------------|---------|---------:|------:|-----------------:|"
      << std::endl;
  for (const auto& [key, result] : results_) {
    std::cout << "|`" << key.implementation << "`|" << key.operation << "|"
              << key.input_size << "|" << absl::StrFormat("%.1f", result.Mean())
              << "±" << absl::StrFormat("%.1f", result.StandardDeviation() * 2)
              << "ns|"
              << absl::StrFormat("%+.1f", result.MinimalMemoryEstimate() *
                                              100.0 / result.MemorySize())
              << "%|" << std::endl;
  }
}

namespace {
constexpr std::string_view kReferenceImplementation = "flatset";
const std::vector<std::string_view> kOtherImplementations = {"SimpleILP", "flatset-nohash"};
const std::vector<std::string_view> kOps = {"insert", "reserved-insert", "contains-found",
                                            "contains-not-found"};

template <class Table, class Key>
const typename Table::value_type& FindOrDie(const Table& table,
                                            const Key& key) {
  auto it = table.find(key);
  if (it == table.end()) {
    abort();
  }
  return *it;
}

std::string TimeString(const BenchmarkResult& result) {
  return absl::StrFormat("%.1f±%.1fns", result.Mean(),
                         result.StandardDeviation() * 2);
}
}  // namespace

void HashBenchmarkResults::Print2(size_t N) const {
  std::cout << "## Benchmark Results: N=" << N << std::endl;
  for (std::string_view op : kOps) {
    PrintOp(op);
  }
}

namespace {
std::string MemoryAmount(double actual) {
  if (actual < 10000) {
    return absl::StrFormat("%.0f", actual);
  }
  if (actual < 10'000'000) {
    return absl::StrFormat("%.0fK", actual / 1000);
  }
  if (actual < 10'000'000'000) {
    return absl::StrFormat("%.0fM", actual / 1'000'000);
  }
  return absl::StrFormat("%.0fG", actual / 1'000'000'000);
}

std::string PercentUp(double reference, double other) {
  return absl::StrFormat("%+.1f%%", 100.0 * (other - reference) / reference);
}
}  // namespace

void HashBenchmarkResults::PrintOp(std::string_view op) const {
  std::set<size_t> sizes;
  for (const auto& [key, result] : results_) {
    if (key.operation == op) {
      sizes.insert(key.input_size);
    }
  }
  std::cout << "### " << op << std::endl << std::endl;
  std::cout << "|Size|" << kReferenceImplementation << " Time/op|";
  for (auto other : kOtherImplementations) {
    std::cout << other << " Time/op|" << other << " Time Change|";
  }
  std::cout << kReferenceImplementation << " Memory|";
  for (auto other : kOtherImplementations) {
    std::cout << other << " Memory|" << other << " Memory Change|";
  }
  std::cout << std::endl;
  std::cout << "|---:|------:|";
  for (auto other [[maybe_unused]] : kOtherImplementations) {
    std::cout << "---:|---:|";
  }
  std::cout << "---:|";
  for (auto other [[maybe_unused]] : kOtherImplementations) {
    std::cout << "---:|---:|";
  }
  std::cout << std::endl;
  for (size_t size : sizes) {
    const auto& [ref_key, ref_result] = FindOrDie(
        results_,
        Key{std::string(kReferenceImplementation), std::string(op), size});
    std::cout << "|" << ref_key.input_size << "|" << TimeString(ref_result)
              << "|";
    for (auto other : kOtherImplementations) {
      const auto& [other_key, other_result] =
          FindOrDie(results_, Key{std::string(other), std::string(op), size});
      std::cout << TimeString(other_result) << "|"
                << PercentUp(ref_result.Mean(), other_result.Mean()) << "|";
    }
    std::cout << MemoryAmount(ref_result.MemorySize()) << "|";
    for (auto other : kOtherImplementations) {
      const auto& [other_key, other_result] =
          FindOrDie(results_, Key{std::string(other), std::string(op), size});
      std::cout << MemoryAmount(other_result.MemorySize()) << "|";
      std::cout << PercentUp(ref_result.MemorySize(), other_result.MemorySize())
                << "|";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

bool operator<(const HashBenchmarkResults::Key& a,
               const HashBenchmarkResults::Key& b) {
  if (a.operation < b.operation) return true;
  if (b.operation < a.operation) return false;
  if (a.input_size < b.input_size) return true;
  if (b.input_size < a.input_size) return false;
  if (a.implementation < b.implementation) return true;
  if (b.implementation < a.implementation) return false;
  return false;
}
