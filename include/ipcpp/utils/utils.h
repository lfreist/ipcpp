//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <ipcpp/utils/platform.h>

#include <chrono>

namespace carry::utils {

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline std::size_t align_up(const std::size_t size, const std::size_t alignment = 16) {
  return (size + alignment - 1) & ~(alignment - 1);
}

template <typename T>
std::string to_string(T& value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

}