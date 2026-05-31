#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mce {

class Bitset {
public:
  Bitset() = default;

  explicit Bitset(std::size_t bits) { resize(bits); }

  void resize(std::size_t bits) {
    bit_count_ = bits;
    words_.assign(word_count(bits), 0ULL);
    trim_tail();
  }

  std::size_t size() const { return bit_count_; }

  void clear() { std::fill(words_.begin(), words_.end(), 0ULL); }

  void set(std::size_t pos) { mutate_bit(pos, true); }

  void reset(std::size_t pos) { mutate_bit(pos, false); }

  bool test(std::size_t pos) const {
    check_position(pos);
    const std::size_t word = pos / word_bits;
    const std::size_t offset = pos % word_bits;
    return (words_[word] & (1ULL << offset)) != 0ULL;
  }

  bool any() const {
    for (auto word : words_) {
      if (word != 0ULL) {
        return true;
      }
    }
    return false;
  }

  bool none() const { return !any(); }

  std::size_t count() const {
    std::size_t total = 0;
    for (auto word : words_) {
#if defined(__GNUG__) || defined(__clang__)
      total += static_cast<std::size_t>(__builtin_popcountll(word));
#else
      while (word != 0ULL) {
        word &= (word - 1ULL);
        ++total;
      }
#endif
    }
    return total;
  }

  std::vector<std::size_t> indices() const {
    std::vector<std::size_t> out;
    out.reserve(count());
    for_each_set_bit([&out](std::size_t pos) { out.push_back(pos); });
    return out;
  }

  // Serialization helpers for distributed protocols
  std::vector<std::uint64_t> words() const { return words_; }

  void set_words(std::size_t bits, const std::vector<std::uint64_t>& w) {
    bit_count_ = bits;
    words_ = w;
    trim_tail();
  }

  template <typename Func>
  void for_each_set_bit(Func&& func) const {
    for (std::size_t word_index = 0; word_index < words_.size(); ++word_index) {
      std::uint64_t word = words_[word_index];
      while (word != 0ULL) {
#if defined(__GNUG__) || defined(__clang__)
        const unsigned long bit = static_cast<unsigned long>(__builtin_ctzll(word));
#else
        unsigned long bit = 0;
        while (((word >> bit) & 1ULL) == 0ULL) {
          ++bit;
        }
#endif
        func(word_index * word_bits + static_cast<std::size_t>(bit));
        word &= (word - 1ULL);
      }
    }
  }

  bool intersects(const Bitset& other) const {
    ensure_same_size(other);
    for (std::size_t i = 0; i < words_.size(); ++i) {
      if ((words_[i] & other.words_[i]) != 0ULL) {
        return true;
      }
    }
    return false;
  }

  Bitset& operator&=(const Bitset& other) {
    ensure_same_size(other);
    for (std::size_t i = 0; i < words_.size(); ++i) {
      words_[i] &= other.words_[i];
    }
    trim_tail();
    return *this;
  }

  Bitset& operator|=(const Bitset& other) {
    ensure_same_size(other);
    for (std::size_t i = 0; i < words_.size(); ++i) {
      words_[i] |= other.words_[i];
    }
    trim_tail();
    return *this;
  }

  Bitset& operator^=(const Bitset& other) {
    ensure_same_size(other);
    for (std::size_t i = 0; i < words_.size(); ++i) {
      words_[i] ^= other.words_[i];
    }
    trim_tail();
    return *this;
  }

  friend Bitset operator&(Bitset lhs, const Bitset& rhs) {
    lhs &= rhs;
    return lhs;
  }

  friend Bitset operator|(Bitset lhs, const Bitset& rhs) {
    lhs |= rhs;
    return lhs;
  }

  friend Bitset operator^(Bitset lhs, const Bitset& rhs) {
    lhs ^= rhs;
    return lhs;
  }

private:
  static constexpr std::size_t word_bits = 64;

  static std::size_t word_count(std::size_t bits) { return (bits + word_bits - 1) / word_bits; }

  void ensure_same_size(const Bitset& other) const {
    if (bit_count_ != other.bit_count_) {
      throw std::invalid_argument("bitset size mismatch");
    }
  }

  void check_position(std::size_t pos) const {
    if (pos >= bit_count_) {
      throw std::out_of_range("bitset index out of range");
    }
  }

  void mutate_bit(std::size_t pos, bool value) {
    check_position(pos);
    const std::size_t word = pos / word_bits;
    const std::size_t offset = pos % word_bits;
    const std::uint64_t mask = 1ULL << offset;
    if (value) {
      words_[word] |= mask;
    } else {
      words_[word] &= ~mask;
    }
  }

  void trim_tail() {
    if (words_.empty()) {
      return;
    }
    const std::size_t remainder = bit_count_ % word_bits;
    if (remainder == 0) {
      return;
    }
    const std::uint64_t mask = (1ULL << remainder) - 1ULL;
    words_.back() &= mask;
  }

  std::size_t bit_count_ = 0;
  std::vector<std::uint64_t> words_;
};

}  // namespace mce