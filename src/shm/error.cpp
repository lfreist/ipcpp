//
// Created by lfreist on 16/10/2024.
//

#include <iostream>

#include <ipcpp/shm/error.h>

// convert given argument to string at compile time
#define TO_STRING(x) #x

namespace ipcpp::shm::error {

std::ostream& operator<<(std::ostream& os, AccessError error) {
  switch (error) {
    case AccessError::ALREADY_LOCKED_FOR_READ:
      os << TO_STRING(AccessError::ALREADY_LOCKED_FOR_READ);
      break;
    case AccessError::ALREADY_LOCKED_FOR_WRITE:
      os << TO_STRING(AccessError::ALREADY_LOCKED_FOR_WRITE);
      break;
    case AccessError::READ_LOCK_TIMEOUT:
      os << TO_STRING(AccessError::READ_LOCK_TIMEOUT);
      break;
    case AccessError::WRITE_LOCK_TIMEOUT:
      os << TO_STRING(AccessError::WRITE_LOCK_TIMEOUT);
      break;
    case AccessError::UNKNOWN_LOCK_ERROR:
      os << TO_STRING(AccessError::UNKNOWN_LOCK_ERROR);
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, MemoryError error) {
  switch (error) {
    case MemoryError::CREATION_ERROR:
      os << TO_STRING(MemoryError::CREATION_ERROR);
      break;
    case MemoryError::ALLOCATION_ERROR:
      os << TO_STRING(MemoryError::ALLOCATION_ERROR);
      break;
    case MemoryError::MAPPING_ERROR:
      os << TO_STRING(MemoryError::MAPPING_ERROR);
      break;
  }
  return os;
}

}
