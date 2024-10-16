//
// Created by lfreist on 16/10/2024.
//

#pragma once

namespace ipcpp::shm::error {

enum class AccessError {
  ALREADY_LOCKED_FOR_READ,
  ALREADY_LOCKED_FOR_WRITE,
  READ_LOCK_TIMEOUT,
  WRITE_LOCK_TIMEOUT,
  UNKNOWN_LOCK_ERROR,
};

enum class MemoryError {
  CREATION_ERROR,    // shm_open fails
  ALLOCATION_ERROR,  // ftruncate fails
  MAPPING_ERROR,     // mmap fails
};

std::ostream& operator<<(std::ostream& os, AccessError error);
std::ostream& operator<<(std::ostream& os, MemoryError error);

}