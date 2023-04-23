#include "benchmark/print_numbers.h"

#include <initializer_list> // for initializer_list
#include <string_view>      // for string_view, operator==, basic_...

#include "absl/log/check.h"
#include "absl/strings/str_format.h"

std::string PrintWithPrecision(double v, size_t precision) {
  std::string s = absl::StrFormat("%f", v);
  std::string_view sv = s;
  if (sv.substr(0, 2) == "0.") {
    CHECK(0);
    return absl::StrFormat("%.*f", precision + 2, v);
  }
  auto position = sv.find(".");
  CHECK_NE(position, std::string_view::npos);
  // There is a decimal (so there is a non-zero value before the
  // decimal.
  if (position >= precision) {
    return absl::StrFormat("%.0f", v);
  }
  return absl::StrFormat("%.*f", precision - position, v);
}

#if 0
namespace {
size_t DivideAndRound(size_t n, size_t m) { return (n + m / 2) / m; }

constexpr size_t kKibi = 1024;
constexpr size_t kMebi = kKibi * kKibi;
constexpr size_t kGibi = kKibi * kMebi;
}  // namespace

std::string NumberWithBinarySuffix(uint64_t n, size_t precision) {
  if (n < 10) return absl::StrFormat("%d", n);
  if (n < 100) {
    if (precision == 1) {
      return absl::StrCat((DivideAndRound(n, 10) * 10));
    }
    return absl::StrCat(n);
  }
  if (n < 1000) {
    if (precision == 1) {
      return absl::StrCat(DivideAndRound(n, 100) * 100);
    }
    if (precision == 2) {
      return absl::StrCat(DivideAndRound(n, 10) * 10);
    }
    return absl::StrCat(n);
  }
  if (n < 10 * kKibi) {
    if (precision == 1) {
      return absl::StrCat(DivideAndRound(n, kKibi), "Ki");
    }
  }
  if (n < 999 * kMebi + kMebi / 2) {
    // Anyt
    goto not_ready;
  }
  if (n < kGibi) {
    // between 1000Mi and 1024Mi
    if (precision == 3) {
      // Produce, e.g., 1023Mi even with though precision is 3.
      return absl::StrCat(DivideAndRound(n, kMebi), "Mi");
    }
    goto not_ready;
  }
  if (n < 10 * kGibi) {
    if (precision == 3) {
      return absl::StrFormat("%4.2fGi", double(n) / kGibi);
    }
  }
 not_ready:
  CHECK(0) << "Not ready to print " << n << " at precision "  << precision;
}
#endif
