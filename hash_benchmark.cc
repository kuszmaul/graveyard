#include "hash_benchmark.h"

#include <cstdlib>  // for abort
#include <random>
#include <vector>

#include "absl/container/flat_hash_set.h"  // for flat_hash_set, BitMask
#include "absl/hash/hash.h"                // for Hash
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"      // for string_view
#include "enum_print.h"
#include "enums_flag.h"

namespace {
const auto* operation_enum_and_strings = EnumsAndStrings<Operation>::Create({{Operation::kInsert, "insert"},
     {Operation::kReservedInsert, "reserved-insert"},
     {Operation::kFound, "found"},
     {Operation::kNotFound, "notfound"}});
}  // namespace

ABSL_FLAG(size_t, size_growth, 100,
          "For benchmarking tables of various sizes, increase the size by "
          "size/size_growth");
ABSL_FLAG(std::vector<Operation>, operations,
          operation_enum_and_strings->Enums(),
          "comma-separated list of operations to benchmark");

std::string AbslUnparseFlag(std::vector<Operation> operations) {
  return AbslUnparseVectorEnumFlag(*operation_enum_and_strings, operations);
}

bool AbslParseFlag(absl::string_view text, std::vector<Operation> *operations, std::string* error) {
  return AbslParseVectorEnumFlag(*operation_enum_and_strings, text, operations, error);
}

bool OperationIsFlagged(Operation operation) {
  std::vector<Operation> operations_vector = absl::GetFlag(FLAGS_operations);
  return absl::c_find(operations_vector, operation) != operations_vector.end();
}

std::string FileNameForHashSetBenchmark(Operation operation, absl::string_view implementation) {
  return absl::StrCat("data/", operation_enum_and_strings->ToString(operation), "_", implementation, ".data");
}

namespace {
std::random_device r;
std::default_random_engine e1(r());
std::uniform_int_distribution<uint64_t> uniform_dist;
}  // namespace

void GetSomeNumbers(size_t size, std::vector<uint64_t>& result) {
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
