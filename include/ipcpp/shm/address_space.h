//
// Created by lfreist on 16/10/2024.
//

#pragma once
#include <fcntl.h>
#include <ipcpp/shm/error.h>
#include <ipcpp/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

namespace ipcpp::shm {

/**
 * RAII class for allocating shared memory on construction and freeing it on destruction
 */
class shared_memory_file {
 public:
  ~shared_memory_file();
  /// move constructor: needed for SharedAddressSpace::create to make a std::expected<SharedAddressSpace>.
  shared_memory_file(shared_memory_file&& other) noexcept;

  static std::expected<shared_memory_file, error::MemoryError> create(std::string&& path, std::size_t size);

  template <AccessMode A>
  static std::expected<shared_memory_file, error::MemoryError> open(std::string&& path);

  [[nodiscard]] int fd() const;
  [[nodiscard]] std::size_t size() const;

  void unlink() const;

 private:
  /// private constructor: only way to get a SharedAddressSpace is to use SharedAddressSpace::create.
  shared_memory_file(std::string&& path, std::size_t size);

 private:
  AccessMode _access_mode = AccessMode::READ;
  std::string _path;
  int _fd = -1;
  std::size_t _size = 0U;
};

// === implementations =================================================================================================
// _____________________________________________________________________________________________________________________
inline std::expected<shared_memory_file, error::MemoryError> shared_memory_file::create(std::string&& path,
                                                                                 const std::size_t size) {
  shared_memory_file self(std::move(path), size);
  self._access_mode = AccessMode::WRITE;

  const int fd = shm_open(self._path.c_str(), O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    return std::unexpected(error::MemoryError::CREATION_ERROR);
  }

  if (ftruncate(fd, static_cast<__off_t>(self._size)) == -1) {
    return std::unexpected(error::MemoryError::ALLOCATION_ERROR);
  }

  self._fd = fd;

  return self;
}

template <AccessMode A>
std::expected<shared_memory_file, error::MemoryError> shared_memory_file::open(std::string&& path) {
  int o_flags = 0;
  if constexpr (A == AccessMode::WRITE) {
    o_flags = O_RDWR;
  } else if constexpr (A == AccessMode::READ) {
    o_flags = O_RDONLY;
  }

  const int fd = shm_open(path.c_str(), o_flags, 0666);
  if (fd == -1) {
    return std::unexpected(error::MemoryError::CREATION_ERROR);
  }

  struct stat shm_stat{};
  if (fstat(fd, &shm_stat) == -1) {
    return std::unexpected(error::MemoryError::CREATION_ERROR);
  }

  shared_memory_file self(std::move(path), shm_stat.st_size);
  self._access_mode = AccessMode::WRITE;

  self._fd = fd;

  return self;
}

}