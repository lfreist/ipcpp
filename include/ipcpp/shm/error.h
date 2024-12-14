//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <iostream>

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

enum class MappingError {
  // mmap error codes - errno is set to one of these errors (probably EINVAL)
  //  read mmap(2) documentation for further details
  MMAP_EACCES = EACCES,
  MMAP_EAGAIN = EAGAIN,
  MMAP_EBADF = EBADF,
  MMAP_EEXIST = EEXIST,
  MMAP_EINVAL = EINVAL,
  MMAP_ENFILE = ENFILE,
  MMAP_ENODEV = ENODEV,
  MMAP_ENOMEM = ENOMEM,
  MMAP_EOVERFLOW = EOVERFLOW,
  MMAP_EPERM = EPERM,
  MMAP_ETXTBSY = ETXTBSY,
  // custom error codes:
  MMAP_FAILED,        // mmap failed
  WRONG_ADDRESS,      // mapped to a different address than requested
  INVALID_HEAP_SIZE,  // issue with heap size: won't fit into address space
  SHM_ERROR,
  UNKNOWN_MAPPING,
};

std::ostream& operator<<(std::ostream& os, AccessError error);
std::ostream& operator<<(std::ostream& os, MemoryError error);
std::ostream& operator<<(std::ostream& os, MappingError error);

}