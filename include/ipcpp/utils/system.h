/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <unistd.h>

#include <concepts>

namespace ipcpp::utils::system {

template <typename T>
  requires std::is_integral_v<T>
inline T round_up_to_pagesize(T val) {
  static std::size_t page_size = sysconf(_SC_PAGESIZE);
  return (val + page_size - 1) & ~(page_size - 1);
}

}  // namespace ipcpp::utils::system