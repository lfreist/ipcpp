/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>
#include <ipcpp/datatypes/fixed_size/basic_string.h>

template <std::size_t N>
struct String {
  char text[N]{};
  std::size_t size = 0;
  static constexpr std::size_t max_size = N;
};