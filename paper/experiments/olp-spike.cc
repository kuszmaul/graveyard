/* Measure the spike for a hovering workload with ordered linear
 * probing. */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hash_function.h"

// This OLP cannot resize.
class Olp {
 public:
  explicit Olp(size_t capacity) :slots_(capacity) {}
  void insert(uint64_t v) {
    const size_t capacity = slots_.size();
    assert(size_ < capacity);
 try_again:
    size_t h1 = H1(v, capacity);
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      bool wrapped = false;
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      const Slot to_insert{!wrapped, v};
      if (!slots_[index].has_value()) {
        ++size_;
        slots_[index] = to_insert;
        return;
      } else {
        assert(slots_[index].value() != v);
        if (to_insert < slots_[index]) {
          v = slots_[index].value();
          slots_[index] = to_insert;
          goto try_again;
        }
      }
    }
    std::cerr << "Table full" << std::endl;
    abort();
  }
  void Validate() const {
    std::cout << "Validating" << std::endl;
    Slot prev;
    size_t count = 0;
    size_t i = 0;
    for (const auto& slot : slots_) {
      if (slot.has_value() && prev.has_value()) {
        assert(prev < slot);
      }
      if (slot.has_value()) {
        ++count;
        prev = slot;
      }
      ++i;
    }
    assert(count == size_);
    std::cout << "Validated" << std::endl;
  }
 private:
  // A slot can be empty (nullopt), or a value, with a bool.  The bool
  // indicates that we didn't wrap around.  We can compare two of the
  // pairs (so that wrapped values sort < unwrapped values).
  class Slot {
   public:
    Slot() :slot_(std::nullopt) {}
    Slot(bool not_wrapped, uint64_t v) :slot_(std::pair<bool, uint64_t>(not_wrapped, v)) {}
    bool has_value() const { return slot_.has_value(); }
    bool wrapped() const {
      assert(slot_);
      return !slot_->first;
    }
    uint64_t value() const {
      assert(slot_);
      return slot_->second;
    }
   private:
    // Requires a and b have values.
    friend bool operator<(const Slot &a, const Slot &b) {
      assert(a.has_value() && b.has_value());
      return a.slot_ < b.slot_;
    }
    friend std::ostream& operator<<(std::ostream& os, const Slot& slot) {
      if (slot.has_value()) {
        if (slot.wrapped()) {
          os << "!";
        }
        return os << (slot.value() >> 54);
      } else {
        return os << "_";
      }
    }
    std::optional<std::pair<bool, uint64_t>> slot_;
  };

  friend std::ostream& operator<<(std::ostream& os, const Olp& olp) {
    os << "{size=" << olp.size_;
    size_t i = 0;
    for (const auto& slot : olp.slots_) {
      os << " ";
      if (slot.has_value()) {
        os << "[" << i << "]";
      }
      os << slot;
      ++i;
    }
    return os << "}";
  }

  // The number of elements in the table
  size_t size_ = 0;
  std::vector<Slot> slots_;
};

class UniqueNumbers {
 public:
  explicit UniqueNumbers(std::mt19937_64 &gen) :gen_(gen) {}
  uint64_t Next() {
    while (true) {
      uint64_t v = distribution_(gen_);
      auto [it, inserted] = seen_.insert(v);
      if (inserted) {
        return v;
      }
    }
  }
 private:
  std::mt19937_64 &gen_;
  std::uniform_int_distribution<uint64_t> distribution_;
  std::unordered_set<uint64_t> seen_;
};

int main() {
  std::mt19937_64 gen;
  constexpr size_t kN = 100;
  UniqueNumbers numbers(gen);
  for (double load_factor : {.95, .98, .99, .995}) {
    Olp olp{kN};
    size_t size = load_factor * kN;
    for (size_t i = 0; i < size; ++i) {
      uint64_t v = numbers.Next();
      olp.insert(v);
      std::cout << "after v=" << v << " (" << (v >> 54) << ") load=" << load_factor << " table=" << olp << std::endl;
      olp.Validate();
      std::cout << "ok" << std::endl;
    }
    std::cout << "load=" << load_factor << " table=" << olp << std::endl;
  }
}
