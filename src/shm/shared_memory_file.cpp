/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/shared_memory_file.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstddef>
#include <functional>

namespace ipcpp::shm {

// === shared_memory_file ==============================================================================================
// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(std::string&& path, const std::size_t size)
    : _path(std::move(path)), _size(size) {}

// _____________________________________________________________________________________________________________________
shared_memory_file::~shared_memory_file() {
  if (_native_handle != -1) {
    close(_native_handle);
  }
}

// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(shared_memory_file&& other) noexcept {
  _path = std::move(other._path);
  _access_mode = other._access_mode;
  std::swap(_size, other._size);
  std::swap(_native_handle, other._native_handle);
}

// _____________________________________________________________________________________________________________________
shared_memory_file& shared_memory_file::operator=(shared_memory_file&& other) noexcept {
  _path = std::move(other._path);
  _access_mode = other._access_mode;
  std::swap(_size, other._size);
  std::swap(_native_handle, other._native_handle);
  return *this;
}

// _____________________________________________________________________________________________________________________
std::string_view shared_memory_file::name() const { return _path; }

// _____________________________________________________________________________________________________________________
std::size_t shared_memory_file::size() const { return _size; }

// _____________________________________________________________________________________________________________________
void shared_memory_file::unlink() const { shm_unlink(_path.c_str()); }

// _____________________________________________________________________________________________________________________
AccessMode shared_memory_file::access_mode() const { return _access_mode; }

#ifdef __linux__
#include "unix/shared_memory_file.impl.cpp"
#endif

}  // namespace ipcpp::shm