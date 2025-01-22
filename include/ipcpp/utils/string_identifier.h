/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace carry {

namespace internal {
constexpr std::size_t max_identifier_size = 128;
}

class StringIdentifier {
 public:
  explicit StringIdentifier(std::string str) : _str(std::move(str)) {
    assert (_str.size() <= internal::max_identifier_size);
  }

  StringIdentifier(StringIdentifier&& other) noexcept : _str(std::move(other._str)) {}
  StringIdentifier(const StringIdentifier& other) = default;

  StringIdentifier& operator=(StringIdentifier&& other) noexcept {
    if (this == std::addressof(other)) {
      return *this;
    }
    _str = std::move(other._str);
    return *this;
  }

  StringIdentifier& operator=(const StringIdentifier& other) {
    if (this == std::addressof(other)) {
      return *this;
    }
    _str = other._str;
    return *this;
  }

  [[nodiscard]] std::string_view name() const { return {_str.data(), _str.size()}; }
  [[nodiscard]] std::size_t size() const { return _str.size(); }

  [[nodiscard]] static std::size_t max_size() { return internal::max_identifier_size; }

 private:
  std::string _str;
};

}