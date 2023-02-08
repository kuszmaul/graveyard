#ifndef ENUMS_FLAG_H_
#define ENUMS_FLAG_H_

// Support for absl flag of a vector of enums.

#include "enum_print.h"

template <class EnumType>
bool AbslParseVectorEnumFlag(const EnumsAndStrings<EnumType>& enums_and_strings,
                             absl::string_view text,
                             std::vector<EnumType>* parsed,
                             std::string* error) {
  std::vector<absl::string_view> op_strings = absl::StrSplit(text, ",");
  std::vector<EnumType> result;
  const size_t number_of_enums = enums_and_strings.Pairs().size();
  for (absl::string_view op_string : op_strings) {
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
    result.push_back(*op);
  }
  *parsed = std::move(result);
  return true;
}

template <class EnumType>
std::string AbslUnparseVectorEnumFlag(
    const EnumsAndStrings<EnumType>& enums_and_strings,
    std::vector<EnumType> operations) {
  return absl::StrJoin(operations, ",", enums_and_strings.Formatter());
}

#endif  // ENUMS_FLAG_H_
