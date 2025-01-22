/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/utils/platform.h>

#ifdef IPCPP_UNIX

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/logging.h>

#include <sys/mman.h>

namespace carry::shm {

// === private definition: map_memory: linux (posix) implementation =====================================================
// _____________________________________________________________________________________________________________________
std::expected<std::uintptr_t, std::error_code> _map_memory(const std::size_t expected_size,
                                                           const std::uintptr_t start_addr,
                                                           const shared_memory_file::native_handle_t file_handle,
                                                           const std::size_t offset, const AccessMode access_mode) {
  const int protect_flags = (access_mode == AccessMode::WRITE) ? PROT_READ | PROT_WRITE : PROT_READ;

  int flags = start_addr ? MAP_FIXED : 0;
  if (file_handle) {
    flags |= MAP_SHARED_VALIDATE;
  } else {
    flags |= MAP_SHARED | MAP_ANONYMOUS;
  }

  void* const mapped_region = ::mmap(reinterpret_cast<void*>(start_addr), expected_size, protect_flags, flags,
                                     file_handle, static_cast<__off_t>(offset));

  if (mapped_region == MAP_FAILED) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapping_error), error_category()));
  }

  if (start_addr && reinterpret_cast<void*>(start_addr) != mapped_region) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapped_at_wrong_address), error_category()));
  }

  return reinterpret_cast<std::uintptr_t>(mapped_region);
}

// === member function definitions: platform specific implementation ===================================================
// ___ msync ___________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::SINGLE>::msync(const bool sync) const {
  ::msync(reinterpret_cast<void*>(_mapped_region), _total_size, sync ? MS_SYNC : MS_ASYNC);
}

// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::DOUBLE>::msync(const bool sync) const {
  ::msync(reinterpret_cast<void*>(_mapped_region), _total_size, sync ? MS_SYNC : MS_ASYNC);
}

// ___ Destructor ______________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::SINGLE>::~MappedMemory() {
  if (_mapped_region != 0) {
    msync(true);
    ::munmap(reinterpret_cast<void*>(_mapped_region), _total_size);
    _mapped_region = 0;
    _size = 0;
    _total_size = 0;
  }
}

// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::DOUBLE>::~MappedMemory() {
  if (_mapped_region != 0) {
    msync(true);
    ::munmap(reinterpret_cast<void*>(_mapped_region), _size);
    ::munmap(reinterpret_cast<void*>(_mapped_region + _size), _size);
    ::munmap(reinterpret_cast<void*>(_mapped_region), _total_size);
    _mapped_region = 0;
    _size = 0;
    _total_size = 0;
  }
}

}  // namespace carry::shm

#endif