//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <chrono>

namespace ipcpp::utils {

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

}