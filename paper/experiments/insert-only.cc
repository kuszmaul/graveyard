// For an insertion-only workload, show the average insertion probe
// length just before hashing, with graveyard and non-graveyard.  This
// is not bucketed.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream> // IWYU pragma: keep
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

#include "../../benchmark/statistics.h"
#include "hash_function.h"

// IWYU note: fstream doesn't quite work right with IWYU.

// A Ratio class that doesn't worry about overflow.
class Ratio {
public:
  Ratio(uint64_t numerator, uint64_t denominator)
      : numerator_(numerator), denominator_(denominator) {}
  explicit Ratio(uint64_t integer) : Ratio(integer, 1) {}
  uint64_t Floor() const { return numerator_ / denominator_; }
  Ratio Invert() const { return Ratio(denominator_, numerator_); }
  double Float() const {
    return static_cast<double>(numerator_) / denominator_;
  }
  friend Ratio operator*(const Ratio &a, const Ratio &b) {
    return Ratio{a.numerator_ * b.numerator_, a.denominator_ * b.denominator_};
  }
  Ratio operator-() const { return Ratio(-numerator_, denominator_); }
  friend Ratio operator+(const Ratio &a, const Ratio &b) {
    return Ratio{a.numerator_ * b.denominator_ + b.numerator_ * a.denominator_,
                 a.denominator_ * b.denominator_};
  }
  friend Ratio operator-(const Ratio &a, const Ratio &b) { return a + (-b); }
  friend Ratio operator/(const Ratio &a, const Ratio &b) {
    return a * b.Invert();
  }
  friend Ratio operator/(uint64_t a, const Ratio &b) {
    return Ratio{a, 1} * b.Invert();
  }
  friend bool operator<(const Ratio &a, const Ratio &b) {
    return a.numerator_ * b.denominator_ < b.numerator_ * a.denominator_;
  }

private:
  uint64_t numerator_, denominator_;
};

struct Slot {
  Slot() : empty(1), search_distance(0), value(0) {}
  uint64_t empty : 1;
  uint64_t search_distance : 63;
  uint64_t value;
};

// Get a 64-bit random number that hasn't been used before.
static uint64_t GetNumber(std::mt19937_64 &generator,
                          std::uniform_int_distribution<uint64_t> &distribution,
                          std::unordered_set<uint64_t> &seen_before) {
  while (true) {
    uint64_t v = distribution(generator);
    auto [it, inserted] = seen_before.insert(v);
    if (inserted) {
      return v;
    }
  }
}

struct AverageProbeLengths {
  Ratio found, notfound, findempty;
  friend std::ostream &operator<<(std::ostream &os,
                                  const AverageProbeLengths &stats) {
    return os << "found=" << stats.found.Float()
              << " notfound=" << stats.notfound.Float()
              << " findempty=" << stats.findempty.Float();
  }
};

class IntSet {
public:
  IntSet(size_t n_slots) : slots_(n_slots, Slot{}) {
    // std::cout << "Initializing to slots=" << n_slots << std::endl;
  }
  // Requires: v is not in the set.
  void insert(uint64_t v, std::optional<size_t> graveyard_period) {
    size_t h1 = H1(v);
    size_t index = h1;
    for (size_t off = 0; off < slots_.size(); ++off, IncrementIndex(index)) {
      if (graveyard_period.has_value() && index % *graveyard_period == 0) {
        // Skip a graveyard slot.
        continue;
      }
      if (slots_[index].empty) {
        ++size_;
        slots_[index].empty = 0;
        slots_[index].value = v;
        slots_[h1].search_distance =
            std::max(slots_[h1].search_distance, off + 1);
        return;
      } else if (slots_[index].value == v) {
        abort();
      }
    }
    std::cerr << "Oops" << std::endl;
    abort();
  }
  AverageProbeLengths GetAverageProbeLengths() const {
    size_t find_cost = 0;
    for (Slot slot : slots_) {
      if (!slot.empty) {
        find_cost += FoundCost(slot.value);
      }
    }
    size_t notfound_cost = 0;
    size_t find_empty_cost = 0;
    for (size_t i = 0; i < slots_.size(); ++i) {
      // std::cout << "slot " << i << " " << NotFoundCost(i) << std::endl;
      notfound_cost += NotFoundCost(i);
      find_empty_cost += FindEmptyCost(i);
    }
    return {.found = Ratio{find_cost, size_},
            .notfound = Ratio{notfound_cost, slots_.size()},
            .findempty = Ratio{find_empty_cost, slots_.size()}};
  }

private:
  void IncrementIndex(size_t &index) const {
    ++index;
    if (index == slots_.size()) {
      index = 0;
    }
  }

  size_t H1(uint64_t v) const { return ::H1(v, slots_.size()); }

  friend std::ostream &operator<<(std::ostream &os, const IntSet &set) {
    os << "{size=" << set.slots_.size() << " ";
    for (size_t i = 0; i < set.slots_.size(); ++i) {
      if (i > 0)
        os << ", ";
      if (set.slots_[i].empty)
        os << "_";
      else
        os << set.H1(set.slots_[i].value);
    }
    return os << "}";
  }

  size_t FoundCost(uint64_t v) const {
    size_t h1 = H1(v);
    size_t index = h1;
    for (size_t off = 0; off < slots_.size(); ++off, IncrementIndex(index)) {
      const Slot &slot = slots_[index];
      if (!slot.empty && slot.value == v) {
        return off + 1;
      }
    }
    std::cerr << "Couldn't find" << std::endl;
    abort();
  }

  size_t NotFoundCost(size_t i) const { return slots_[i].search_distance; }

  size_t FindEmptyCost(size_t i) const {
    size_t index = i;
    for (size_t off = 0; off < slots_.size(); ++off, IncrementIndex(index)) {
      if (slots_[index].empty) {
        return off + 1;
      }
    }
    std::cerr << "No empties" << std::endl;
    abort();
  }

  size_t size_ = 0;
  std::vector<Slot> slots_;
};

struct ProbeStatistics {
  Statistics found, notfound, findempty;
};

AverageProbeLengths MeasureOnce(std::optional<size_t> graveyard_period,
                                Ratio load_factor, Ratio additional_load_factor,
                                std::mt19937_64 &generator) {
  std::uniform_int_distribution<uint64_t> distribution;
  // double x = (Ratio(1)-(load_factor +
  // additional_load_factor)).Invert().Float();
  std::unordered_set<uint64_t> seen_numbers;
  std::vector<uint64_t> first_numbers;
  std::vector<uint64_t> second_numbers;
  constexpr size_t n_elements = 1'000'000;
  for (size_t i = 0; i < n_elements; ++i) {
    first_numbers.push_back(GetNumber(generator, distribution, seen_numbers));
  }
  for (size_t i = 0; Ratio(i) < additional_load_factor * Ratio(n_elements);
       ++i) {
    second_numbers.push_back(GetNumber(generator, distribution, seen_numbers));
  }
  IntSet set((n_elements / load_factor).Floor());
  for (auto v : first_numbers) {
    set.insert(v, graveyard_period);
  }
  // std::cout << "Got " << set << std::endl;
  // std::cout << "second size = " << second_numbers.size() << std::endl;
  for (auto v : second_numbers) {
    set.insert(v, std::nullopt);
  }
  // std::cout << "Then " << set << std::endl;
  AverageProbeLengths stats = set.GetAverageProbeLengths();
  return stats;
}

// Takes `generator` by value so that two different calls to `Measure`
// will produce the same random numbers.
ProbeStatistics Measure(std::optional<size_t> graveyard_period,
                        Ratio load_factor, Ratio additional_load_factor,
                        std::mt19937_64 generator) {
  ProbeStatistics result;
  for (size_t i = 0; i < 20; ++i) {
    AverageProbeLengths lengths = MeasureOnce(
        graveyard_period, load_factor, additional_load_factor, generator);
    result.found.AddDatum(lengths.found.Float());
    result.notfound.AddDatum(lengths.notfound.Float());
    result.findempty.AddDatum(lengths.findempty.Float());
  }
  return result;
}

// Measures the load increasing from 90% to 99% where, on rehash, we
// fill have the remaining space with graveyard tombstones (or not if
// we are measuring no-tombstones), and then insert to fill 1/4 of the
// empty space.
void MeasureIncreasingLoad(std::mt19937_64 generator) {
  std::ofstream ofile;
  ofile.open("increasing-load.data");
  ofile << "#initial-fullness final-fullness X (X+1)/2 (X^2+1)/2"
        << " no-tombstones-found-mean no-tombstones-found-sigma "
           "no-tombstones-notfound-mean no-tombstones-notfound-sigma "
           "no-tombstones-findempty-mean no-tombstones-findempty-sigma"
        << " graveyard-found-mean graveyard-found-sigma "
           "graveyard-notfound-mean graveyard-notfound-sigma "
           "graveyard-findempty-mean graveyard-findempty-sigma"
        << std::endl;
  std::vector<Ratio> fills;
  for (size_t i = 90; i <= 99; ++i) {
    fills.push_back(Ratio{i, 100});
  }
  fills.push_back(Ratio{199, 200});
  for (Ratio fillto : fills) {
    Ratio remaining = Ratio{1} - fillto;
    Ratio tombstones = remaining * Ratio{1, 2};
    Ratio additional = remaining * Ratio{1, 4};
    ProbeStatistics no_tombstones =
        Measure(std::nullopt, fillto, additional, generator);
    ProbeStatistics graveyard =
        Measure(tombstones.Invert().Floor(), fillto, additional, generator);
    double X = (Ratio{1} - (fillto + additional)).Invert().Float();
    ofile << fillto.Float() << " " << (fillto + additional).Float() << " " << X
          << " " << (X + 1) / 2 << " " << (X * X + 1) / 2 << " "
          << no_tombstones.found.Mean() << " "
          << no_tombstones.found.StandardDeviation() << " "
          << no_tombstones.notfound.Mean() << " "
          << no_tombstones.notfound.StandardDeviation() << " "
          << no_tombstones.findempty.Mean() << " "
          << no_tombstones.findempty.StandardDeviation() << " "
          << graveyard.found.Mean() << " "
          << graveyard.found.StandardDeviation() << " "
          << graveyard.notfound.Mean() << " "
          << graveyard.notfound.StandardDeviation() << " "
          << graveyard.findempty.Mean() << " "
          << graveyard.findempty.StandardDeviation() << std::endl;
  }
}

// Measures the load, starting at 98% full, with varying amounts of
// tombstones, and then insert 0.5% more values that can use the
// tombstones.  Then measure the probe lengths.
void MeasureReducingGraveyard(std::mt19937_64 generator) {
  std::ofstream ofile;
  ofile.open("vary-tombstones.data");
  Ratio fillto = Ratio{98, 100};
  Ratio remaining = Ratio{1} - fillto;
  Ratio additional = remaining * Ratio{1, 4};
  ofile << "#Graveyard-factor found-mean found-sigma notfound-mean "
           "notfound-sigma findempty-mean findempty-sigma"
        << std::endl;
  // Graveyard period cannot go as low as 50
  for (size_t graveyard_period :
       {65, 70, 75, 80, 90, 100, 120, 140, 160, 180, 200, 220, 240, 260}) {
    ProbeStatistics graveyard =
        Measure(graveyard_period, fillto, additional, generator);
    // E.g., when fillto is 98% (so we will add an additional 0.5%)
    // and the tombstone period is 100 (1/100 = 1%) we want 2 =
    // 1%/0.5%.
    std::cout << "additional=" << additional.Float()
              << " 1/gp=" << Ratio{1, graveyard_period}.Float() << std::endl;
    ofile << (Ratio{1, graveyard_period} / additional).Float() << " "
          << graveyard.found.Mean() << " "
          << graveyard.found.StandardDeviation() << " "
          << graveyard.notfound.Mean() << " "
          << graveyard.notfound.StandardDeviation() << " "
          << graveyard.findempty.Mean() << " "
          << graveyard.findempty.StandardDeviation() << std::endl;
  }
}

int main() {
  // Generate the same sequence of random numbers for all tables.  So
  // pass `gen` by value.
  std::mt19937_64 gen;
  MeasureIncreasingLoad(gen);
  MeasureReducingGraveyard(gen);
#if 0
  //Measure("test", 10, Ratio{3, 5}, Ratio{1, 5}, gen);
  Measure("90% full, no tombstones, add 2.5%", std::nullopt, Ratio{90, 100}, Ratio{25, 1000}, gen);
  // 5% graveyard, 90% full
  Measure("90% full, 5% tombstones, add 2.5%      ", 20, Ratio{90, 100}, Ratio{25, 1000}, gen);
  Measure("94% full, no tombstones, add 2%        ", std::nullopt, Ratio{94, 100}, Ratio{2, 100}, gen);
  Measure("94% full, 4% tombstones, add 2%        ", 25, Ratio{94, 100}, Ratio{2, 100}, gen);
  Measure("96% full, no tombstones, add 1%        ", std::nullopt, Ratio{96, 100}, Ratio{1, 100}, gen);
  Measure("96% full, 2% tombstones, add 1%        ", 50, Ratio{96, 100}, Ratio{1, 100}, gen);
  Measure("97% full, no tombstones, add 1%        ", std::nullopt, Ratio{97, 100}, Ratio{1, 100}, gen);
  Measure("97% full, 2% tombstones, add 1%        ", 50, Ratio{97, 100}, Ratio{1, 100}, gen);
  Measure("98% full, no tombstones, add 0.5%      ", std::nullopt, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 1% tombstones, add 0.5%      ", 100, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.8% tombstones, add 0.5%    ", 125, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.625% tombstones, add 0.5%  ", 160, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.5% tombstones, add 0.5%    ", 200, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.4% tombstones, add 0.5%    ", 225, Ratio{98, 100}, Ratio{1, 200}, gen);
#endif
}
