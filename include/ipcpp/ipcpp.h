//
// Created by leon- on 16/12/2024.
//

#pragma once

#include <ipcpp/shm/factory.h>
#include <ipcpp/stl/allocator.h>

#include <cstdint>

namespace ipcpp {

inline bool initialize_dynamic_buffer(std::size_t size = 0) {
  if (size == 0) {
    while (true) {
      const auto expected_memory = shm::shared_memory::open<AccessMode::WRITE>("/ipcpp.dyn.shm");
      if (expected_memory.has_value()) {
        pool_allocator<std::uint8_t>::initialize_factory(reinterpret_cast<void*>(expected_memory.value().addr()),
                                                         expected_memory.value().size());
        return true;
      }
      std::cerr << "Failed to open shared memory region." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  } else {
    const auto expected_memory = shm::shared_memory::open_or_create("/ipcpp.dyn.shm", size);
    if (!expected_memory.has_value()) {
      return false;
    }
    pool_allocator<std::uint8_t>::initialize_factory(reinterpret_cast<void*>(expected_memory.value().addr()),
                                                     expected_memory.value().size());
    return true;
  }
}

}  // namespace ipcpp