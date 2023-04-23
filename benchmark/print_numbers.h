#ifndef BENCHMARK_PRINT_NUMBERS_H_
#define BENCHMARK_PRINT_NUMBERS_H_

#include <cstddef> // for size_t
#include <string>  // for string

// std::string NumberWithBinarySuffix(uint64_t n, size_t precision);

// Returns a string with enough digits after the decimal to give us a
// total of `precision` digits.
std::string PrintWithPrecision(double n, size_t precision);

#endif // BENCHMARK_PRINT_NUMBERS_H_
