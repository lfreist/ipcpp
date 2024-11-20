/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <iostream>

namespace ipcpp::io {

template <class CharT, class Traits>
std::size_t getline(std::basic_istream<CharT, Traits>& input, std::span<CharT> dest, CharT delim = '\n') {
  std::size_t counter = 0;
  for (auto& c : dest) {
    if (!input.get(c)) {
      break;
    }
    if (c == delim) {
      break;
    }
    counter++;
  }
  return counter;
}

}