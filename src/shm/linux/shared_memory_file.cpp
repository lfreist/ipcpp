/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/utils/platform.h>

#ifdef IPCPP_UNIX

#include <ipcpp/shm/shared_memory_file.h>
#include <ipcpp/utils/utils.h>

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace carry::shm {

// _____________________________________________________________________________________________________________________
shared_memory_file::~shared_memory_file() {
  if (_native_handle != 0) {
    close(_native_handle);
  }
  if (_was_created) {
    unlink();
  }
}

// _____________________________________________________________________________________________________________________
void shared_memory_file::unlink() const { shm_unlink(_path.c_str()); }

// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::create(std::string&& path,
                                                                              std::size_t size) {
  size = utils::align_up(size, getpagesize());
  shared_memory_file self(std::move(path), size);
  self._access_mode = AccessMode::WRITE;

  shm_unlink(self._path.c_str());

  const int fd = shm_open(self._path.c_str(), O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::creation_error), error_category()));
  }

  if (ftruncate(fd, static_cast<std::int64_t>(self._size)) == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::resize_error), error_category()));
  }

  self._native_handle = fd;
  self._was_created = true;

  return self;
}

// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::open(std::string&& path, const AccessMode access_mode) {
  const int o_flags = (access_mode == AccessMode::WRITE) ? O_RDWR : O_RDONLY;

  const int fd = shm_open(path.c_str(), o_flags, 0666);
  if (fd == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::open_error), error_category()));
  }

  struct stat shm_stat{};
  if (fstat(fd, &shm_stat) == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::file_not_found), error_category()));
  }

  shared_memory_file self(std::move(path), shm_stat.st_size);
  self._access_mode = access_mode;

  self._native_handle = fd;

  return self;
}

}

#endif