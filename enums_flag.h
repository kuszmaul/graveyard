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
std::string EnumFlagErrorString(const EnumsAndStrings<EnumType> &enums_and_strings,
				std::string_view text) {
  std::string result = absl::StrCat(text, " is not one of");
  const size_t number_of_enums = enums_and_strings.Pairs().size();
  for (size_t i = 0; i < number_of_enums; ++i) {
    if (i > 0) {
      absl::StrAppend(&result, ", ");
    }
    if (i + 1 == number_of_enums) {
      absl::StrAppend(&result, " or ");
    }
    absl::StrAppend(&result, " '", enums_and_strings.Pairs()[i].second, "'");
  }
  return result;
}

template <class EnumType>
bool AbslParseSetEnumFlag(const EnumsAndStrings<EnumType> &enums_and_strings,
			  std::string_view text,
			  absl::flat_hash_set<EnumType> *parsed,
			  std::string *error) {
  std::vector<std::string_view> op_strings = absl::StrSplit(text, ",");
  absl::flat_hash_set<EnumType> result;
  for (std::string_view op_string : op_strings) {
    std::optional<EnumType> op = enums_and_strings.ToEnum(op_string);
    if (!op) {
      *error = EnumFlagErrorString(enums_and_strings, op_string);
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

template <class EnumType>
bool AbslParseEnumFlag(const EnumsAndStrings<EnumType> &enums_and_strings,
		       std::string_view text,
		       EnumType *parsed,
		       std::string *error) {
  std::optional<EnumType> enum_value = enums_and_strings.ToEnum(text);
  if (enum_value) {
    *parsed = *enum_value;
    return true;
  } else {
    *error = EnumFlagErrorString(enums_and_strings, text);
    return false;
  }
}

template <class EnumType>
std::string
AbslUnparseEnumFlag(const EnumsAndStrings<EnumType> &enums_and_strings,
		    EnumType v) {
  return std::string(enums_and_strings.ToString(v));
}

#endif // ENUMS_FLAG_H_
