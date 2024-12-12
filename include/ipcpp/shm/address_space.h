//
// Created by lfreist on 16/10/2024.
//

#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

#include <ipcpp/shm/error.h>
#include <ipcpp/types.h>

namespace ipcpp::shm {

/**
 * RAII class for allocating shared memory on construction and freeing it on destruction
 */
class SharedAddressSpace {
 public:
  ~SharedAddressSpace();
  /// move constructor: needed for SharedAddressSpace::create to make an std::expected<SharedAddressSpace>.
  SharedAddressSpace(SharedAddressSpace&& other) noexcept;

  template <AccessMode A>
  static std::expected<SharedAddressSpace, error::MemoryError> create(std::string&& path, std::size_t size);

  [[nodiscard]] int fd() const;
  [[nodiscard]] void* addr() const;
  [[nodiscard]] std::size_t size() const;

  void unlink();

 private:
  /// private constructor: only way to get a SharedAddressSpace is to use SharedAddressSpace::create.
  SharedAddressSpace(std::string&& path, std::size_t size);

 private:
  AccessMode _access_mode = AccessMode::READ;
  std::string _path;
  int _fd = -1;
  void* _start = nullptr;
  std::size_t _size = 0U;
};

// === implementations =================================================================================================
// _____________________________________________________________________________________________________________________
template <AccessMode A>
std::expected<SharedAddressSpace, error::MemoryError> SharedAddressSpace::create(std::string&& path, std::size_t size) {
  int o_flags = 0;
  if constexpr (A == AccessMode::WRITE) {
    o_flags = O_RDWR | O_CREAT;
  } else if constexpr (A == AccessMode::READ) {
    o_flags = O_RDONLY;
  }

  SharedAddressSpace self(std::move(path), size);
  self._access_mode = A;

  const int fd = shm_open(self._path.c_str(), o_flags, 0666);
  if (fd == -1) {
    return std::unexpected(error::MemoryError::CREATION_ERROR);
  }

  if constexpr (A == AccessMode::WRITE) {
    if (ftruncate(fd, static_cast<__off_t>(self._size)) == -1) {
      return std::unexpected(error::MemoryError::ALLOCATION_ERROR);
    }
  }

  self._fd = fd;

  return self;
}

}