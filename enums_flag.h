#ifndef ENUMS_FLAG_H_
#define ENUMS_FLAG_H_

// Support for absl flag of a vector of enums.

#include <cstddef>  // for size_t
#include <optional> // for optional
#include <string>   // for string
#include <string_view>
#include <vector> // for vector

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h" // for StrAppend, StrCat
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "enum_print.h"

template <class EnumType>
bool AbslParseSetEnumFlag(const EnumsAndStrings<EnumType> &enums_and_strings,
			  std::string_view text,
			  absl::flat_hash_set<EnumType> *parsed,
			  std::string *error) {
  std::vector<std::string_view> op_strings = absl::StrSplit(text, ",");
  absl::flat_hash_set<EnumType> result;
  const size_t number_of_enums = enums_and_strings.Pairs().size();
  for (std::string_view op_string : op_strings) {
    std::optional<EnumType> op = enums_and_strings.ToEnum(op_string);
    if (!op) {
      *error = absl::StrCat(op_string, " is not one of");
      for (size_t i = 0; i < number_of_enums; ++i) {
        if (i > 0) {
          absl::StrAppend(error, ", ");
        }
        if (i + 1 == number_of_enums) {
          absl::StrAppend(error, " or ");
        }
        absl::StrAppend(error, " '", enums_and_strings.Pairs()[i].second, "'");
      }
      return false;
    }
    result.insert(*op);
  }
  *parsed = std::move(result);
  return true;
}

template <class EnumType>
std::string
AbslUnparseSetEnumFlag(const EnumsAndStrings<EnumType> &enums_and_strings,
		       absl::flat_hash_set<EnumType> operations) {
  return absl::StrJoin(operations, ",", enums_and_strings.Formatter());
}

#endif // ENUMS_FLAG_H_
