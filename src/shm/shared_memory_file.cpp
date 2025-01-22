/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/shm/shared_memory_file.h>

#include <chrono>
#include <cstddef>

namespace carry::shm {

// === shared_memory_file ==============================================================================================
// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(std::string&& path, const std::size_t size)
    : _path(std::move(path)), _size(size) {}

// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(shared_memory_file&& other) noexcept {
  _path = std::move(other._path);
  _access_mode = other._access_mode;
  std::swap(_size, other._size);
  std::swap(_native_handle, other._native_handle);
  std::swap(_was_created, other._was_created);
}

// _____________________________________________________________________________________________________________________
shared_memory_file& shared_memory_file::operator=(shared_memory_file&& other) noexcept {
  _path = std::move(other._path);
  _access_mode = other._access_mode;
  std::swap(_size, other._size);
  std::swap(_native_handle, other._native_handle);
  std::swap(_was_created, other._was_created);
  return *this;
}

// _____________________________________________________________________________________________________________________
const std::string& shared_memory_file::name() const { return _path; }

// _____________________________________________________________________________________________________________________
std::size_t shared_memory_file::size() const { return _size; }

// _____________________________________________________________________________________________________________________
AccessMode shared_memory_file::access_mode() const { return _access_mode; }

// _____________________________________________________________________________________________________________________
shared_memory_file::native_handle_t shared_memory_file::native_handle() const {
  return _native_handle;
}

}  // namespace carry::shm