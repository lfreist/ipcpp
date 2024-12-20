/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/utils/platform.h>

#ifdef IPCPP_WINDOWS

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/logging.h>

#include <expected>

#define NOMINMAX
#include <windows.h>

namespace ipcpp::shm {

// === private definition: map_memory: windows implementation ==========================================================
// _____________________________________________________________________________________________________________________
std::expected<std::uintptr_t, std::error_code> _map_memory(const std::size_t expected_size,
                                                           const std::uintptr_t start_addr,
                                                           const shared_memory_file::native_handle_t file_handle,
                                                           const std::size_t offset, const AccessMode access_mode) {
  if (!file_handle) {
    // we create a VirtualAlloc
    void* reserved_region = VirtualAlloc(reinterpret_cast<void*>(start_addr), expected_size, MEM_RESERVE, PAGE_READWRITE);
    if (!reserved_region) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::mapping_error), error_category()));
    }
    return reinterpret_cast<std::uintptr_t>(reserved_region);
  }

  const DWORD desired_access = (access_mode == AccessMode::WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;

  LPCTSTR mapped_region;
  if (start_addr == 0) {
    mapped_region = static_cast<LPTSTR>(MapViewOfFile(file_handle, desired_access, static_cast<DWORD>(offset >> 32),
                                                      static_cast<DWORD>(offset & 0xFFFFFFFF), expected_size));
  } else {
    mapped_region = static_cast<LPTSTR>(
        MapViewOfFileEx(file_handle, desired_access, static_cast<DWORD>(offset >> 32),
                        static_cast<DWORD>(offset & 0xFFFFFFFF), expected_size, reinterpret_cast<LPVOID>(start_addr)));
  }
  if (mapped_region == nullptr) {
    UnmapViewOfFile(mapped_region);
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapping_error), error_category()));
  }

  if (start_addr && reinterpret_cast<LPCTSTR>(start_addr) != mapped_region) {
    UnmapViewOfFile(mapped_region);
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapped_at_wrong_address), error_category()));
  }

  return reinterpret_cast<std::uintptr_t>(mapped_region);
}

// === member function definitions: platform specific implementation ===================================================
// ___ msync ___________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::SINGLE>::msync(const bool sync) const {}

// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::DOUBLE>::msync(const bool sync) const {}

// ___ Destructor ______________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::SINGLE>::~MappedMemory() {
  if (_mapped_region != 0) {
    // if MT is MappingType::SINGLE, MapViewOfFile was used for mapping memory
    UnmapViewOfFile(reinterpret_cast<LPCTSTR>(_mapped_region));
    _mapped_region = 0;
    _size = 0;
    _total_size = 0;
  }
}

// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::DOUBLE>::~MappedMemory() {
  if (_mapped_region != 0) {
    // if MT is MappingType::DOUBLE, MapViewOfFileEx was used for mapping memory second instance
    UnmapViewOfFileEx(reinterpret_cast<PVOID>(_mapped_region), 0x00000001);
    UnmapViewOfFileEx(reinterpret_cast<PVOID>(_mapped_region + _size), 0x00000001);
    VirtualFree(reinterpret_cast<void*>(_mapped_region), _total_size, MEM_RELEASE);
    _mapped_region = 0;
    _size = 0;
    _total_size = 0;
  }
}

}  // namespace ipcpp::shm

#endif