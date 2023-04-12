// For an insertion-only workload, show the average insertion probe
// length just before hashing, with graveyard and non-graveyard.  This
// is not bucketed.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

// A Ratio class that doesn't worry about overflow.
class Ratio {
 public:
  Ratio(uint64_t numerator, uint64_t denominator) :numerator_(numerator), denominator_(denominator) {}
  explicit Ratio(uint64_t integer) : Ratio(integer, 1) {}
  uint64_t Floor() const {
    return numerator_ / denominator_;
  }
  Ratio Invert() const {
    return Ratio(denominator_, numerator_);
  }
  double Float() const {
    return static_cast<double>(numerator_) / denominator_;
  }
  friend Ratio operator*(const Ratio& a, const Ratio &b) {
    return Ratio{a.numerator_ * b.numerator_, a.denominator_ * b.denominator_};
  }
  Ratio operator-() const {
    return Ratio(-numerator_, denominator_);
  }
  friend Ratio operator+(const Ratio& a, const Ratio &b) {
    return Ratio{a.numerator_ * b.denominator_ + b.numerator_ * a.denominator_, a.denominator_ * b.denominator_};
  }
  friend Ratio operator-(const Ratio& a, const Ratio &b) {
    return a + (-b);
  }
  friend Ratio operator/(const Ratio& a, const Ratio &b);
  friend Ratio operator/(uint64_t a, const Ratio &b) {
    return Ratio{a, 1} * b.Invert();
  }
  friend bool operator<(const Ratio& a, const Ratio& b) {
    return a.numerator_ * b.denominator_ < b.numerator_ * a.denominator_;
  }
 private:
  uint64_t numerator_, denominator_;
};

// Get a non-zero random number that hasn't been used before.
static uint64_t GetNumber(std::mt19937_64 &generator, std::uniform_int_distribution<uint64_t>& distribution, std::unordered_set<uint64_t>& seen_before) {
  while (true) {
    uint64_t v = distribution(generator);
    if (v == 0) {
      continue;
    }
    auto [it, inserted] = seen_before.insert(v);
    if (inserted) {
      return v;
    }
  }
}

struct ProbeStatistics {
  Ratio found, notfound;
  friend std::ostream& operator<<(std::ostream& os, const ProbeStatistics& stats) {
    return os << "found=" << stats.found.Float() << " notfound=" << stats.notfound.Float();
  }
};

// This set uses 0 as "empty".
class IntSet {
 public:
  IntSet(size_t n_slots) :slots_(n_slots, 0) {
    // std::cout << "Initializing to slots=" << n_slots << std::endl;
  }
  // Requires: v is not in the set.
  void insert(uint64_t v, std::optional<size_t> graveyard_period) {
    size_t h1 = v % slots_.size();
    for (size_t off = 0; off < slots_.size(); ++off) {
      size_t index = (h1 + off);
      if (index >= slots_.size()) index -= slots_.size();
      if (graveyard_period.has_value() && index % *graveyard_period == 0) {
        // Skip a graveyard slot.
        continue;
      }
      if (slots_[index] == 0) {
        ++size_;
        slots_[index] = v;
        return;
      } else if (slots_[index] == v) {
        abort();
      }
    }
    std::cerr << "Oops" << std::endl;
    abort();
  }
  ProbeStatistics GetProbeStatistics() const {
    size_t find_cost = 0;
    for (uint64_t v : slots_) {
      if (v != 0) {
        find_cost += FoundCost(v);
      }
    }
    size_t notfound_cost = 0;
    for (size_t i = 0; i < slots_.size(); ++i) {
      notfound_cost += NotFoundCost(i);
    }
    return {.found = Ratio{find_cost, size_},
            .notfound = Ratio{notfound_cost, slots_.size()}};
  }
 private:
  friend std::ostream& operator<<(std::ostream &os, const IntSet &set) {
    os << "{";
    for (size_t i = 0; i < set.slots_.size(); ++i) {
      if (i > 0) os << ", ";
      if (set.slots_[i] == 0) os << "_";
      else os << set.slots_[i] % 1000;
    }
    return os << "}";
  }

  size_t FoundCost(uint64_t v) const {
    assert(v != 0);
    size_t h1 = v % slots_.size();
    for (size_t off = 0; off < slots_.size(); ++off) {
      size_t index = (h1 + off);
      if (index >= slots_.size()) index -= slots_.size();
      if (slots_[index] == v) {
        return off + 1;
      }
    }
    std::cerr << "Couldn't find" << std::endl;
    abort();
  }

  size_t NotFoundCost(size_t i) const {
    for (size_t off = 0; off < slots_.size(); ++off) {
      size_t index = (i + off);
      if (index >= slots_.size()) index -= slots_.size();
      if (slots_[index] == 0) {
        return off + 1;
      }
    }
    std::cerr << "All full" << std::endl;
    abort();
  }

  size_t size_ = 0;
  std::vector<uint64_t> slots_;
};

void Measure(std::string_view description, std::optional<size_t> graveyard_period, Ratio load_factor, Ratio additional_load_factor, std::mt19937_64 generator) {
  double x = (Ratio(1)-(load_factor + additional_load_factor)).Invert().Float();
  double xp1o2 = (x + 1) / 2;
  double x2p1o2 = (x * x + 1) / 2;
  std::uniform_int_distribution<uint64_t> distribution;
  std::unordered_set<uint64_t> seen_numbers;
  std::vector<uint64_t> first_numbers;
  std::vector<uint64_t> second_numbers;
  constexpr size_t n_elements = 1'000'000;
  for (size_t i = 0; i < n_elements; ++i) {
    first_numbers.push_back(GetNumber(generator, distribution, seen_numbers));
  }
  for (size_t i = 0; Ratio(i) < additional_load_factor * Ratio(n_elements); ++i) {
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
  ProbeStatistics stats = set.GetProbeStatistics();
  std::cout << description << "(Knuth predicts " << xp1o2 << " " << x2p1o2 << "):" << stats << std::endl;
}

int main() {
  // Generate the same sequence of random numbers for both tables.
  std::mt19937_64 gen;
  Measure("90% full, no tombstones, add 2.5%", std::nullopt, Ratio{90, 100}, Ratio{25, 1000}, gen);
  // 5% graveyard, 90% full
  Measure("90% full, 5% tombstones, add 2.5%", 20, Ratio{90, 100}, Ratio{25, 1000}, gen);
  Measure("94% full, no tombstones, add 2%  ", std::nullopt, Ratio{94, 100}, Ratio{2, 100}, gen);
  Measure("94% full, 4% tombstones, add 2%  ", 25, Ratio{94, 100}, Ratio{2, 100}, gen);
  Measure("96% full, no tombstones, add 1%  ", std::nullopt, Ratio{96, 100}, Ratio{1, 100}, gen);
  Measure("96% full, 2% tombstones, add 1%  ", 50, Ratio{96, 100}, Ratio{1, 100}, gen);
  Measure("97% full, no tombstones, add 1%  ", std::nullopt, Ratio{97, 100}, Ratio{1, 100}, gen);
  Measure("97% full, 2% tombstones, add 1%  ", 50, Ratio{97, 100}, Ratio{1, 100}, gen);
  Measure("98% full, no tombstones, add 0.5%  ", std::nullopt, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 1% tombstones, add 0.5%  ", 100, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.8% tombstones, add 0.5%  ", 125, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.625% tombstones, add 0.5%  ", 160, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.5% tombstones, add 0.5%  ", 200, Ratio{98, 100}, Ratio{1, 200}, gen);
  Measure("98% full, 0.4% tombstones, add 0.5%  ", 225, Ratio{98, 100}, Ratio{1, 200}, gen);
}
