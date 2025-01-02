//
// Created by leon- on 16/12/2024.
//

#pragma once

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/stl/allocator.h>

#include <cstdint>
#include <chrono>

using namespace std::chrono_literals;

namespace ipcpp {

inline std::error_code initialize_runtime(const std::size_t size = 0) {
  if (size != 0) {
    if (auto global_topic = get_topic("global", size); global_topic) {
      pool_allocator<std::uint8_t>::initialize_factory(global_topic.value()->shm().addr(),
                                                       global_topic.value()->shm().size());
      return {0, std::system_category()};
    } else {
      return global_topic.error();
    }
  } else {
    if (auto global_topic = get_topic("global"); global_topic) {
      pool_allocator<std::uint8_t>::initialize_factory(global_topic.value()->shm().addr(),
                                                       global_topic.value()->shm().size());
      return {};
    } else {
      return global_topic.error();
    }
  }
}

}  // namespace ipcpp