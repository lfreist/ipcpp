/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>
#include <array>
#include <string>

namespace ipcpp {

namespace internal {
constexpr std::size_t max_identifier_size = 128;
}

template <std::size_t N = internal::max_identifier_size>
class StringIdentifier {
 public:
  StringIdentifier(const char (&str)[N-1]) {
    static_assert(N-1 <= internal::max_identifier_size);
    _str.resize(N-1);
    for (int i = 0; i < (N-1); ++i) {
      _str[i] = str[i];
    }
  }

  [[nodiscard]] std::string_view name() const { return std::string_view(_str.data(), _str.size()); }
  [[nodiscard]] std::size_t size() const { return _str.size(); }

  [[nodiscard]] std::size_t max_size() const { return internal::max_identifier_size; }

 private:
  std::string _str;
};

}