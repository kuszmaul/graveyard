#ifndef BENCHMARK_STATISTICS_H_
#define BENCHMARK_STATISTICS_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

// Compute statistics for a set of data values: min, max, mean, standard
// deviation.
class Statistics {
public:
  void AddDatum(double v) {
    min_ = min_ ? std::min(*min_, v) : v;
    max_ = max_ ? std::max(*max_, v) : v;
    x_sum_ += v;
    x_squared_sum_ += v * v;
    ++n_;
  }
  // Requires at least one sample
  double Mean() const { return x_sum_ / n_; }
  // Requires at least one sample
  double Min() const { return *min_; }
  // Requires at least one sample
  double Max() const { return *max_; }
  // Requires at least one sample
  double StandardDeviation() const {
    return sqrt(x_squared_sum_ / n_ - square(x_sum_ / n_));
  }

private:
  static double square(double x) { return x * x; }
  std::optional<double> min_ = std::nullopt;
  std::optional<double> max_ = std::nullopt;
  double x_sum_ = 0;
  double x_squared_sum_ = 0;
  size_t n_ = 0;
};

#endif // BENCHMARK_STATISTICS_H_
