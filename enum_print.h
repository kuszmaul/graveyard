#ifndef ENUM_PRINT_H_
#define ENUM_PRINT_H_

// Support for parsing and printing an enum

// IWYU has some strange behavior around std::pair.  It wants to get rid
// of utility and add iterator.

#include <optional>
#include <string>  // for string
#include <utility> // IWYU pragma: keep
#include <vector>

// IWYU pragma: no_include <iterator>

#include "absl/log/check.h"
#include "absl/strings/string_view.h"

template <class EnumType> class EnumsAndStrings {
public:
  // Requires that `pairs` contain all a pair for every enum.  If called more
  // than once, `pairs` should be the same every time it is called.
  static const EnumsAndStrings *
  Create(std::vector<std::pair<EnumType, absl::string_view>> pairs) {
    // No static variables with nontrivial destructors.
    static const EnumsAndStrings *enums_and_strings =
        new EnumsAndStrings(std::move(pairs));
    return enums_and_strings;
  }

  absl::string_view ToString(EnumType enum_type) const {
    for (const auto &[e, s] : pairs_) {
      if (e == enum_type)
        return s;
    }
    CHECK(false) << "This shouldn't happen.  Perhaps there is a missing pair "
                    "when calling the EnumsAndStrings constructor";
  }
  std::optional<EnumType> ToEnum(absl::string_view string) const {
    for (const auto &[e, s] : pairs_) {
      if (s == string)
        return e;
    }
    return std::nullopt;
  }
  // Suitable to use as the formatter in absl::StrJoin
  class JoinFormatter {
  public:
    JoinFormatter(const EnumsAndStrings &enums_and_strings)
        : enums_and_strings_(&enums_and_strings) {}
    void operator()(std::string *out, EnumType enum_type) {
      out->append(enums_and_strings_->ToString(enum_type));
    }

  private:
    const EnumsAndStrings *enums_and_strings_;
  };
  JoinFormatter Formatter() const { return JoinFormatter(*this); }
  const std::vector<std::pair<EnumType, absl::string_view>> &Pairs() const {
    return pairs_;
  }
  absl::flat_hash_set<EnumType> Enums() const {
    absl::flat_hash_set<EnumType> result;
    for (const auto &[e, s] : pairs_) {
      result.insert(e);
    }
    return result;
  }

private:
  EnumsAndStrings(std::vector<std::pair<EnumType, absl::string_view>> pairs)
      : pairs_(std::move(pairs)) {}

  std::vector<std::pair<EnumType, absl::string_view>> pairs_;
};

#endif // ENUM_PRINT_H_
