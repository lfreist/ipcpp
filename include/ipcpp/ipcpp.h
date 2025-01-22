/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/stl/allocator.h>

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace carry {

inline std::error_code initialize_runtime(const std::size_t size = 0) {
  if (size != 0) {
    if (auto global_topic = get_shm_entry("global", size); global_topic) {
      pool_allocator<std::uint8_t>::initialize_factory(global_topic.value()->shm().addr(),
                                                       global_topic.value()->shm().size());
      return {0, std::system_category()};
    } else {
      return global_topic.error();
    }
  } else {
    if (auto global_topic = get_shm_entry("global"); global_topic) {
      pool_allocator<std::uint8_t>::initialize_factory(global_topic.value()->shm().addr(),
                                                       global_topic.value()->shm().size());
      return {};
    } else {
      return global_topic.error();
    }
  }
}

}  // namespace carry