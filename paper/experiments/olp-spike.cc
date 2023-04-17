/* Measure the spike for a hovering workload with ordered linear
 * probing. */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "hash_function.h"

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

struct ProbeLengths {
  double found;
  double notfound;
  double insert;
};

// A 64-bit integer that prints as a short number (easier to see)
struct PV {
  uint64_t v;
  explicit PV(uint64_t v) :v(v) {}
  friend std::ostream& operator<<(std::ostream& os, PV pv) {
    return os << (pv.v >> 54) << "." << std::hex << (pv.v % 16) << std::dec;
  }
};


// This OLP cannot resize.
class Olp {
 public:
  explicit Olp(size_t capacity) :slots_(capacity), nominal_capacity_(capacity) {}
  bool insert(uint64_t v) {
    assert(size_ <= nominal_capacity_);
    for (size_t index = H1(v, nominal_capacity_); true; ++index) {
      assert(index <= slots_.size());
      if (index == slots_.size()) {
        slots_.push_back(Slot{});
      }
      Slot& slot = slots_[index];
      switch (slot.tag()) {
        case Tag::kEmpty: {
          ++size_;
          slot = Slot(v);
          return true;
        }
        case Tag::kPresent: {
          if (v == slot.value()) {
            return false;
          }
          if (v < slot.value()) {
            std::swap(v, slot.value());
          }
          continue;
        }
        case Tag::kTombstone: {
          if (v <= slot.value() ||
              index + 1 == slots_.size() ||
              slots_[index + 1].tag() == Tag::kEmpty ||
              slots_[index + 1].value() > v) {
            // If the tombstone is big enough, or if the next slot is
            // big enough or empty (or non-existent) we can use this
            // slot.
            ++size_;
            slot = Slot(v);
            return true;
          }
          continue;
        }
      }
    }
  }
  bool erase(uint64_t v) {
    for (size_t index = H1(v, nominal_capacity_); index < slots_.size(); ++index) {
      Slot& slot = slots_[index];
      switch (slot.tag()) {
        case Tag::kEmpty: {
          return false;
        }
        case Tag::kPresent: {
          if (v == slot.value()) {
            --size_;
            if (index + 1 == slots_.size() ||
                slots_[index + 1].tag() == Tag::kEmpty ||
                H1(slots_[index + 1].value(), nominal_capacity_) == index + 1) {
              // This slot can just be an empty.  This can occur if
              //  * If it's the end of the array,
              //  * If the next slot is empty, or
              //  * If the next slot is at its preferred slot.

              slot = Slot();
            } else {
              slot.ConvertToTombstone();
            }
            return true;
          }
          if (v < slot.value()) {
            return false;
          }
          continue;
        }
        case Tag::kTombstone: {
          if (v <= slot.value()) {
            return false;
          }
          continue;
        }
      }
    }
    return false;
  }

  bool contains(uint64_t v) const {
    return Contains(v).first;
  }

  void Validate() const {
    Slot prev;
    size_t count = 0;
    size_t i = 0;
    for (const auto& slot : slots_) {
      if (!slot.empty() && !prev.empty()) {
        assert(prev < slot);
      }
      if (slot.present()) {
        ++count;
        prev = slot;
        if (!contains(slot.value())) {
          std::cout << "Looking for " << (slot.value() >> 54) << " in " << *this << std::endl;
        }
        assert(contains(slot.value()));
      }
      if (slot.tombstone()) {
        assert(!contains(slot.value()));
      }
      ++i;
    }
    assert(count == size_);
  }

  ProbeLengths GetProbeLengths() const {
    return ProbeLengths{
      .found = FoundAverageProbeLength(),
      .notfound = NotFoundAverageProbeLength(),
      .insert = InsertAverageProbeLength()
    };
  }

 private:
  // Computes the average probe length for a successful lookup by
  // iterating through the present values and determining the probe
  // length to find each value.
  double FoundAverageProbeLength() const {
    double n = 0;
    size_t found_sum = 0;
    for (const auto& slot : slots_) {
      if (slot.present()) {
        auto [found, length] = Contains(slot.value());
        assert(found);
        ++n;
        found_sum += length;
      }
    }
    return found_sum / n;
  }

  // Computes the average probe length for an unsuccessful lookup.
  // For each slot, we find the range of slots that we must actually
  // look at, take the average (since a lookup could end at any
  // particular non-empty slot (based on the hash getting too big),
  // and we assume that it's equal probability that it ends at any
  // particular slot.
  double NotFoundAverageProbeLength() const {
    double sum = 0;
    for (size_t i = 0; i < nominal_capacity_; ++i) {
      sum += NotFoundAverageProbeLength(i);
      //std::cout << "this_sum=" << this_sum << " this_n=" << this_n << std::endl;
    }
    return sum / nominal_capacity_;
  }

  // Computes the average probe length for an unsuccessful lookup that
  // starts at slot `i`.
  double NotFoundAverageProbeLength(const size_t i) const {
    //std::cout << __LINE__ << ":Looking at " << i << std::endl;
    double this_sum = 0;
    size_t this_n = 0;
    for (size_t j = i; j < slots_.size(); ++j) {
      const Slot& slot = slots_[j];
      switch (slot.tag()) {
        case Tag::kEmpty: {
          this_sum += (j - i) + 1;
          ++this_n;
          return this_sum / this_n;
        }
        case Tag::kTombstone:
        case Tag::kPresent: {
          size_t h1 = H1(slot.value(), nominal_capacity_);
          if (h1 > i) {
            this_sum += (j - i) + 1;
            ++this_n;
            //std::cout << __LINE__ << ": at " << j << " this_sum=" << this_sum << " this_n=" << this_n << " slots' h1=" << h1 << std::endl;
            return this_sum / this_n;
          }
          if (h1 == i) {
            this_sum += (j - i) + 1;
            ++this_n;
            //std::cout << __LINE__ << ": at " << j << " this_sum=" << this_sum << " this_n=" << this_n << " slots' h1=" << h1 << std::endl;
          } else {
            //std::cout << __LINE__ << ": at " << j << " slots'h1=" << h1 << std::endl;
          }
          break;
        }
      }
    }
    //std::cout << __LINE__ << ": " << i << " off the end this_sum=" << this_sum << " this_n=" << this_n << std::endl;
    //std::cout << *this << std::endl;
    if (this_n == 0) {
      // If we haven't already looked at the last slot, we need to
      // account for the fact that we must look at all the slots.
      this_sum += (slots_.size() - i);
      ++this_n;
    }
    return this_sum / this_n;
  }

  double InsertAverageProbeLength() const {
    double sum = 0;
    for (size_t i = 0; i < nominal_capacity_; ++i) {
      sum += InsertAverageProbeLength(i);
    }
    return sum / nominal_capacity_;
  }

  double InsertAverageProbeLength(const size_t i) const {
    double sum = 0;
    size_t n = 0;
    for (size_t j = i; j < slots_.size(); ++j) {
      const Slot& slot = slots_[j];
      switch (slot.tag()) {
        case Tag::kEmpty: {
          sum += (j - i) + 1;
          ++n;
          return sum / n;
        }
        case Tag::kTombstone:
        case Tag::kPresent: {
          size_t h1 = H1(slot.value(), nominal_capacity_);
          if (h1 >= i) {
            sum += (j - i) + distance_to_non_present(j);
            ++n;
          }
          if (h1 > i) {
            return sum / n;
          }
          continue;
        }
      }
    }
    if (n == 0) {
      // If we haven't already looked at the last slot we need to account for it.
      sum += (slots_.size() - i);
      ++n;
    }
    return sum / n;
  }

  // Returns the number of slots you must examine, starting at i,
  // before you find a non-present slot.  If the first slot is
  // non-empty, then returns 1, for example.
  size_t distance_to_non_present(size_t i) const {
    for (size_t j = i; j < slots_.size(); ++j) {
      const Slot& slot = slots_[j];
      if (!slot.present()) {
        return j - i  + 1;
      }
    }
    return slots_.size() - i;
  }

#if 0
  double InsertAverageProbeLength(UniqueNumbers &numbers) const {
    const size_t capacity = slots_.size();
    const size_t kNumberOfTrails = capacity * 16;
    for (size_t i = 0; i < kNumberOfTrials; ++i) {


    for (size_t i = 0; i < capacity; ++i) {
      sum += InsertAverageProbeLength(i);
    }
    return sum / capacity;
  }

  double InsertAverageProbeLength(size_t slot_number) const {
    const size_t capacity = slots_.size();
    for (size_t off = 0; off < capacity; ++off) {
      size_t index = h1 + off;
      if (index >= capacity) {
        index -= capacity;
      }

  }
#endif

#if 0
  double InsertAverageProbeLength(UniqueNumbers &unique_numbers) const {
    const size_t kN = slots_.size() * 10;
    double n = 0;
    double sum = 0;
    for (size_t i = 0; i < kN; ++i) {
      sum += InsertProbeLength(unique_numbers.Next());
    }
    return sum / kN;
  }
  double InsertProbeLength(uint64_t v) const {
    const size_t capacity = slots_.size();
    assert(size_ < capacity);
 try_again:
    size_t h1 = H1(v, capacity);
    size_t total_off;
    for (size_t off = 0; off < capacity; ++off, ++total_off) {
      size_t index = h1 + off;
      bool wrapped = false;
      if (index >= capacity) {
        index -= capacity;
        wrapped = true;
      }
      Slot& slot = slots_[index];
      const Slot to_insert{wrapped, v};
      switch (slot.tag()) {
        case Tag::kEmpty: {
          return total_off;
        }
        case Tag::kPresent: {
          assert(slot.value() != v);
          if (to_insert < slot) {
            v = slot.value();
            goto try_again;


  }
#endif

  // Returns a `bool` (`true` if `v` is present) and a `size_t` (the
  // number of slots examined).
  std::pair<bool, size_t/*probe_length*/> Contains(uint64_t v) const {
    size_t probe_length = 1;
    for (size_t index = H1(v, nominal_capacity_); index < slots_.size(); ++index, ++probe_length) {
      const Slot& slot = slots_[index];
      switch (slot.tag()) {
        case Tag::kEmpty: {
          return {false, probe_length};
        }
        case Tag::kPresent: {
          if (v == slot.value()) {
            return {true, probe_length};
          }
          if (v < slot.value()) {
            return {false, probe_length};
          }
          continue;
        }
        case Tag::kTombstone: {
          if (v <= slot.value()) {
            return {false, probe_length};
          }
          continue;
        }
      }
    }
    return {false, probe_length};
  }

  // A slot can be empty (nullopt), or a non-empty.  For non-empty it
  // can be present or contain a tombstone.  In the non-empty case
  // there is a bool indicating that it wrapped and a integer value.
  // The bool is there so we can compare two pairs: wrapped values
  // sort before unwrapped valueds.

  class Slot {
   public:
    enum Tag {kEmpty, kPresent, kTombstone};

    Slot() :tag_{kEmpty} {}
    explicit Slot(uint64_t v) :tag_{kPresent}, value_(v) {}
    bool empty() const { return tag_ == kEmpty; }
    bool present() const { return tag_ == kPresent; }
    bool tombstone() const { return tag_ == kTombstone; }
    // Requires not empty.
    uint64_t value() const {
      assert(tag_ != kEmpty);
      return value_;
    }
    uint64_t& value() {
      assert(tag_ != kEmpty);
      return value_;
    }
    Tag tag() const {
      return tag_;
    }
    void ConvertToTombstone() {
      assert(present());
      tag_ = kTombstone;
    }
   private:
    // Requires a and b not empty.
    friend bool operator<(const Slot &a, const Slot &b) {
      return a.value_ < b.value_;
    }
    friend std::ostream& operator<<(std::ostream& os, const Slot& slot) {
      if (!slot.empty()) {
        if (slot.tombstone()) {
          os << "T";
        }
        return os << PV(slot.value());
      } else {
        return os << "_";
      }
    }
    Tag tag_;
    uint64_t value_;
  };

  using Tag = Slot::Tag;

  friend std::ostream& operator<<(std::ostream& os, const Olp& olp) {
    os << "{size=" << olp.size_ << " nominal_cap=" << olp.nominal_capacity_;
    size_t i = 0;
    for (const auto& slot : olp.slots_) {
      os << " ";
      if (!slot.empty()) {
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
  // We don't wrap, so `slots_` could be bigger than the capacity
  // specified at the time of construction.  `nominal_capacity_` is
  // the specified-at-construction capacity.
  size_t nominal_capacity_;
};

uint64_t RemoveRandom(std::vector<uint64_t> &values, std::mt19937_64 &gen) {
  std::uniform_int_distribution<uint64_t> distrib(0, values.size() - 1);
  uint64_t index = distrib(gen);
  assert(index < values.size());
  uint64_t v = values[index];
  values[index] = values.back();
  values.pop_back();
  return v;
}

void test() {
  std::mt19937_64 gen;
  constexpr size_t kN = 100;
  UniqueNumbers numbers(gen);
  for (double load_factor : {.95, .98, .99, .995}) {
    Olp olp{kN};
    std::vector<uint64_t> values;
    size_t size = load_factor * kN;
    for (size_t i = 0; i < size; ++i) {
      uint64_t v = numbers.Next();
      olp.insert(v);
      values.push_back(v);
      //std::cout << __LINE__ << ":after v=" << (v >> 54) << " load=" << load_factor << " table=" << olp << std::endl;
      olp.Validate();
      //std::cout << "ok" << std::endl;
    }
    // std::cout << "load=" << load_factor << " table=" << olp << std::endl;
    uint64_t v = RemoveRandom(values, gen);
    {
      bool erased = olp.erase(v);
      // std::cout << "Erased " << (v >> 54) << " Got " << olp << std::endl;
      assert(erased);
    }
    olp.insert(v);
    {
      bool erased = olp.erase(v);
      assert(erased);
    }
    olp.insert(v-1);
    // std::cout << __LINE__ << ": Inserted and Erased and inserted - 1 " << (v >> 54) << " Got " << olp << std::endl;
    olp.erase(v-1);
    // std::cout << __LINE__ << ":" << std::endl;
    olp.insert(v);
    // std::cout << __LINE__ << ": Inserted " << (v >> 54) << " Got " << olp << std::endl;
    olp.Validate();
    // std::cout << __LINE__ << ":" << std::endl;
    {
      bool erased = olp.erase(v);
      assert(erased);
    }
    olp.insert(v+1);
    // std::cout << "Inserted and Erased and inserted + 1 " << (v >> 54) << " Got " << olp << std::endl;
    olp.erase(v+1);
    olp.Validate();
  }
}

int main() {
  test();
  std::mt19937_64 master_gen;
  constexpr size_t kN = 1'000'000;
  for (double load_factor : {.95, .98, .99, .995}) {
    std::ofstream ofile;
    ofile.open("spike-" + std::to_string(kN) + "-" + std::to_string(load_factor) + ".data");
    ofile << "#insert_number found notfound insert" << std::endl;
    std::cout << "load=" << load_factor << std::endl;
    std::mt19937_64 gen = master_gen;
    UniqueNumbers unique_numbers(gen);
    Olp olp{kN};
    std::vector<uint64_t> values;
    size_t size = load_factor * kN;
    size_t report_every = size / 200;
    size_t i_mod_report_every = 0;
    auto do_an_insert = [&](size_t i) {
      uint64_t v = unique_numbers.Next();
      olp.insert(v);
      values.push_back(v);
      if (i_mod_report_every == 0) {
        ProbeLengths probelengths = olp.GetProbeLengths();
        ofile << i << " " << probelengths.found << " " << probelengths.notfound << " " << probelengths.insert << std::endl;
      }
      ++i_mod_report_every;
      if (i_mod_report_every == report_every) {
        i_mod_report_every = 0;
      }
    };
    for (size_t i = 0; i < size; ++i) {
      do_an_insert(i);
    }
    for (size_t i = size; i < 2 * size; ++i) {
      {
        uint64_t v = RemoveRandom(values, gen);
        bool erased = olp.erase(v);
        assert(erased);
      }
      do_an_insert(i);
    }
  }
}
