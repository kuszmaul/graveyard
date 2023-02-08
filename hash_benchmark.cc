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
#include "enum_flag.h"

ABSL_FLAG(size_t, size_growth, 100,
          "For benchmarking tables of various sizes, increase the size by "
          "size/size_growth");

namespace {
using OpFlag = EnumFlag<Operation, Operation::kNotFound>;
}  // namespace

namespace {
const std::array kAllOperations{Operation::kInsert, Operation::kReservedInsert, Operation::kFound, Operation::kNotFound};
}  // namespace
ABSL_FLAG(std::vector<Operation>, operations,
          OpFlag.All(),
          "comma-separated list of operations to benchmark");

absl::string_view ToString(Operation operation) {
  switch (operation) {
    case Operation::kInsert: return "insert";
    case Operation::kReservedInsert: return "reserved-insert";
    case Operation::kFound: return "found";
    case Operation::kNotFound: return "notfound";
  }
  CHECK(false) << "This shouldn't happen";
}

namespace{
std::optional<Operation> OperationFromString(absl::string_view operation) {
  for (Operation op : kAllOperations) {
    if (ToString(op) == operation) {
      return op;
    }
  }
  return std::nullopt;
}

struct OperationFormatter {
  void operator()(std::string*out, Operation operation) {
    out->append(ToString(operation));
  }
};
}  // namespace

std::string AbslUnparseFlag(std::vector<Operation> operations) {
  return absl::StrJoin(operations, ",", OperationFormatter());
}

bool AbslParseFlag(absl::string_view text, std::vector<Operation> *operations, std::string* error) {
  std::vector<absl::string_view> op_strings = absl::StrSplit(text, ",");
  std::vector<Operation> result;
  for (absl::string_view op_string : op_strings) {
    std::optional<Operation> op = OperationFromString(op_string);
    if (!op) {
      *error = absl::StrCat(op_string, " is not one of");
      for (size_t i = 0; i < kAllOperations.size(); ++i) {
        if (i > 0) {
          absl::StrAppend(error, ", ");
        }
        if (i + 1 == kAllOperations.size()) {
          absl::StrAppend(error, " or ");
        }
        absl::StrAppend(error, " '", ToString(kAllOperations[i]), "'");
      }
      return false;
    }
    result.push_back(*op);
  }
  *operations = std::move(result);
  return true;
}

absl::flat_hash_set<Operation>& GetOperations() {
  std::vector<Operation> operations_vector = absl::GetFlag(FLAGS_operations);
  static absl::flat_hash_set<Operation> operations(operations_vector.begin(), operations_vector.end());
  return operations;
}

std::string FileNameForHashSetBenchmark(Operation operation, absl::string_view implementation) {
  return absl::StrCat("data/", ToString(operation), "_", implementation);
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
