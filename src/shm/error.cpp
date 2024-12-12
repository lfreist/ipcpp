//
// Created by lfreist on 16/10/2024.
//

#include <ipcpp/shm/error.h>

#include <iostream>

// convert given argument to string at compile time
#define TO_STRING(x) #x

namespace ipcpp::shm::error {

std::ostream& operator<<(std::ostream& os, const AccessError error) {
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

std::ostream& operator<<(std::ostream& os, const MemoryError error) {
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

std::ostream& operator<<(std::ostream& os, const MappingError error) {
  switch (error) {
    case MappingError::MMAP_EACCES:
      os << TO_STRING(MappingError::MMAP_EACCES);
      break;
    case MappingError::MMAP_EAGAIN:
      os << TO_STRING(MappingError::MMAP_EAGAIN);
      break;
    case MappingError::MMAP_EBADF:
      os << TO_STRING(MappingError::MMAP_EBADF);
      break;
    case MappingError::MMAP_EEXIST:
      os << TO_STRING(MappingError::MMAP_EEXIST);
      break;
    case MappingError::MMAP_EINVAL:
      os << TO_STRING(MappingError::MMAP_EINVAL);
      break;
    case MappingError::MMAP_ENFILE:
      os << TO_STRING(MappingError::MMAP_ENFILE);
      break;
    case MappingError::MMAP_ENODEV:
      os << TO_STRING(MappingError::MMAP_ENODEV);
      break;
    case MappingError::MMAP_ENOMEM:
      os << TO_STRING(MappingError::MMAP_ENOMEM);
      break;
    case MappingError::MMAP_EOVERFLOW:
      os << TO_STRING(MappingError::MMAP_EOVERFLOW);
      break;
    case MappingError::MMAP_EPERM:
      os << TO_STRING(MappingError::MMAP_EPERM);
      break;
    case MappingError::MMAP_ETXTBSY:
      os << TO_STRING(MappingError::MMAP_ETXTBSY);
      break;
    case MappingError::MMAP_FAILED:
      os << TO_STRING(MappingError::MMAP_FAILED);
      break;
    case MappingError::WRONG_ADDRESS:
      os << TO_STRING(MappingError::WRONG_ADDRESS);
      break;
    case MappingError::INVALID_HEAP_SIZE:
      os << TO_STRING(MappingError::INVALID_HEAP_SIZE);
      break;
    case MappingError::SHM_ERROR:
      os << TO_STRING(MappingError::SHM_ERROR);
      break;
  }
  return os;
}

}  // namespace ipcpp::shm::error
