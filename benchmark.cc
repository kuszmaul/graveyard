#include "benchmark.h"

#include <time.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>                      // for getline, string

#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/strings/string_view.h" // for string_view

ABSL_FLAG(size_t, number_of_trials, 5, "Number of trials.  Must be positive.");

namespace {
inline uint64_t operator-(struct timespec a, struct timespec b) {
  return (a.tv_sec - b.tv_sec) * 1'000'000'000ul + a.tv_nsec - b.tv_nsec;
}

void VerifyPerformanceGovernor() {
  std::ifstream infile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
  std::string line;
  CHECK(std::getline(infile, line));
  CHECK_EQ(line, "performance")
      << "CPU governor wrong, fix as \"echo performance | sudo tee "
         "/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor\"";
}
} // namespace

void Benchmark(std::ofstream &output,
               std::function<void(size_t count, size_t trial)> setup,
               std::function<size_t()> fun, const std::vector<size_t> &counts) {
  VerifyPerformanceGovernor();
  const size_t kNumberOfTrials = absl::GetFlag(FLAGS_number_of_trials);
  const double kNInverse = 1.0 / kNumberOfTrials;
  for (size_t count : counts) {
    uint64_t x_sum = 0;
    uint64_t x_squared_sum = 0;
    size_t memory_size = 0;
    for (size_t trial = 0; trial < kNumberOfTrials; ++trial) {
      setup(count, trial);
      struct timespec start;
      clock_gettime(CLOCK_MONOTONIC, &start);
      memory_size = fun();
      DoNotOptimize(memory_size);
      struct timespec end;
      clock_gettime(CLOCK_MONOTONIC, &end);
      uint64_t elapsed = end - start;
      x_sum += elapsed;
      x_squared_sum += elapsed * elapsed;
      // if (trial > 0) {
      //  Throw away the time of the first call to fun(), but keep the
      //  memory_size.
      output << "#" << count << "," << elapsed << "," << memory_size
             << std::endl;
      //}
    }
    // These variables are named as if in Reverse Polish Notation.
    double x_expected = x_sum * kNInverse;
    double x_expected_square = x_expected * x_expected;
    double x_square_expected = x_squared_sum * kNInverse;
    double standard_deviation = sqrt(x_square_expected - x_expected_square);
    output << count << "," << x_expected << "," << memory_size << ","
           << 2 * standard_deviation << std::endl;
  }
}

