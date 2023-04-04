#include "hash_benchmark.h"

#include <cstdlib> // for abort
#include <random>
#include <string_view>
#include <utility> // for pair, move

#include "absl/container/flat_hash_set.h" // for flat_hash_set, BitMask
#include "absl/hash/hash.h"               // for Hash
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h" // for string_view
#include "enum_print.h"
#include "enums_flag.h"

namespace {
const auto *operation_enum_and_strings = EnumsAndStrings<Operation>::Create(
    {{Operation::kInsert, "insert"},
     {Operation::kReservedInsert, "reserved-insert"},
     {Operation::kFound, "found"},
     {Operation::kNotFound, "notfound"}});
} // namespace

ABSL_FLAG(size_t, size_growth, 20,
          "For benchmarking tables of various sizes, increase the size by "
          "size/size_growth");
ABSL_FLAG(absl::flat_hash_set<Operation>, operations,
          operation_enum_and_strings->Enums(),
          "comma-separated list of operations to benchmark");

std::string AbslUnparseFlag(absl::flat_hash_set<Operation> operations) {
  return AbslUnparseSetEnumFlag(*operation_enum_and_strings, operations);
}

bool AbslParseFlag(std::string_view text, absl::flat_hash_set<Operation> *operations,
                   std::string *error) {
  return AbslParseSetEnumFlag(*operation_enum_and_strings, text, operations,
			      error);
}

bool OperationIsFlagged(Operation operation) {
  return absl::GetFlag(FLAGS_operations).contains(operation);
}

std::string FileNameForHashSetBenchmark(Operation operation,
                                        std::string_view implementation) {
  return absl::StrCat("data/", operation_enum_and_strings->ToString(operation),
                      "_", implementation, ".data");
}

namespace {
std::random_device r;
std::default_random_engine e1(r());
std::uniform_int_distribution<uint64_t> uniform_dist;
} // namespace

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

std::vector<uint64_t>
GetSomeOtherNumbers(const std::vector<uint64_t> &other_numbers) {
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
const typename Table::value_type &FindOrDie(const Table &table,
                                            const Key &key) {
  auto it = table.find(key);
  if (it == table.end()) {
    abort();
  }
  return *it;
}
} // namespace

const EnumsAndStrings<Implementation> *implementation_enum_and_strings =
    EnumsAndStrings<Implementation>::Create(
        {{Implementation::kGraveyard, "graveyard"},
         {Implementation::kGoogle, "google"},
         {Implementation::kFacebook, "facebook"},
         {Implementation::kOLP, "OLP"},
         {Implementation::kGraveyardIdentityHash, "graveyard-idhash"},
         {Implementation::kGoogleIdentityHash, "google-idhash"},
         {Implementation::kFacebookIdentityHash, "facebook-idhash"},
         {Implementation::kOLPIdentityHash, "OLP-idhash"},
         {Implementation::kGraveyard3578, "graveyard3578"},
         {Implementation::kGraveyard1278, "graveyard1278"},
         {Implementation::kGraveyard2345, "graveyard2345"},
	 {Implementation::kGraveyard9092, "graveyard9092"},
	 {Implementation::kGraveyard9092NoGraveyard, "graveyard9092NoGraveyard"},
         {Implementation::kGraveyard255, "graveyard255"}});

bool AbslParseFlag(std::string_view text, Implementation *implementation, std::string *error) {
  return AbslParseEnumFlag(*implementation_enum_and_strings, text, implementation, error);
}
std::string AbslUnparseFlag(Implementation implementation) {
  return AbslUnparseEnumFlag(*implementation_enum_and_strings, implementation);
}

ABSL_FLAG(absl::flat_hash_set<Implementation>, implementations,
          implementation_enum_and_strings->Enums(),
          "comma-separated list of hash table implementations to benchmark");

std::string AbslUnparseFlag(absl::flat_hash_set<Implementation> implementations) {
  return AbslUnparseSetEnumFlag(*implementation_enum_and_strings,
                                   implementations);
}

bool AbslParseFlag(std::string_view text,
                   absl::flat_hash_set<Implementation> *implementations,
                   std::string *error) {
  return AbslParseSetEnumFlag(*implementation_enum_and_strings, text,
			      implementations, error);
}

std::string_view ImplementationString(Implementation implementation) {
  return implementation_enum_and_strings->ToString(implementation);
}
