#include "benchmark.h"

#include <time.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>                      // for getline, string

#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/strings/string_view.h" // for string_view
#include "benchmark/statistics.h"

ABSL_FLAG(size_t, number_of_trials, 5, "Number of trials.  Must be positive.");

namespace {

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
  for (size_t count : counts) {
    Statistics statistics;
    size_t memory_size = 0;
    for (size_t trial = 0; trial < kNumberOfTrials; ++trial) {
      setup(count, trial);
      timespec start = GetTime();
      memory_size = fun();
      DoNotOptimize(memory_size);
      struct timespec end = GetTime();
      uint64_t elapsed = end - start;
      statistics.AddDatum(elapsed);
      // if (trial > 0) {
      //  Throw away the time of the first call to fun(), but keep the
      //  memory_size.
      output << "#" << count << "," << elapsed << "," << memory_size
             << std::endl;
      //}
    }
    // These variables are named as if in Reverse Polish Notation.
    //
    output << count << "," << statistics.Mean() << "," << memory_size << ","
           << 2 * statistics.StandardDeviation() << std::endl;
  }
}

