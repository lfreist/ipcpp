//
// Created by leon- on 17/12/2024.
//

#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/platform.h>

#ifdef IPCPP_WINDOWS

#include <ipcpp/shm/shared_memory_file.h>

#define NOMINMAX
#include <windows.h>

namespace ipcpp::shm {

// _____________________________________________________________________________________________________________________
shared_memory_file::~shared_memory_file() {
  if (_native_handle) {
    CloseHandle(_native_handle);
  }
}

// _____________________________________________________________________________________________________________________
void shared_memory_file::unlink() const {}

// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::create(std::string&& path,
                                                                              const std::size_t size) {
  shared_memory_file self(std::move(path), size);
  self._access_mode = AccessMode::WRITE;

  HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, static_cast<DWORD>(size >> 32), static_cast<DWORD>(size & 0xffffffff), self.name().c_str());

  if (handle == nullptr) {
    std::unexpected(std::error_code(static_cast<int>(error_t::creation_error), error_category()));
  }

  self._native_handle = handle;

  return self;
}

// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::open(std::string&& path, const AccessMode access_mode) {
  DWORD desired_access = (access_mode == AccessMode::WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;

  HANDLE handle = OpenFileMapping(desired_access, FALSE, path.c_str());

  if (handle == nullptr) {
    std::cout << GetLastError() << std::endl;
    std::unexpected(std::error_code(static_cast<int>(error_t::open_error), error_category()));
  }

  // windows shm is a little f***ed: we cannot retrieve a size here because the shm is not backed by a file
  //  Workaround: we map the handle and try to read its size:

  const void* mapped_buffer = MapViewOfFile(handle, desired_access, 0, 0, 0);
  if (mapped_buffer == nullptr) {
    CloseHandle(handle);
    return std::unexpected(std::error_code(static_cast<int>(error_t::open_error), error_category()));
  }

  MEMORY_BASIC_INFORMATION info;
  if (VirtualQuery(mapped_buffer, &info, sizeof(info)) == 0) {
    UnmapViewOfFile(mapped_buffer);
    CloseHandle(handle);
    return std::unexpected(std::error_code(static_cast<int>(error_t::open_error), error_category()));
  }

  logging::debug("shared_memory_file::open: read size of {} for {}", info.RegionSize, std::string(path));

  shared_memory_file self(std::move(path), info.RegionSize);
  UnmapViewOfFile(mapped_buffer);

  self._access_mode = access_mode;

  self._native_handle = handle;

  return self;
}

}

#endif