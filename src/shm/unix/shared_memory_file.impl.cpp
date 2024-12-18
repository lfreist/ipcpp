/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

// _____________________________________________________________________________________________________________________
int shared_memory_file::native_handle() const {
  return _native_handle;
}


// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::create(std::string&& path,
                                                                              const std::size_t size) {
  shared_memory_file self(std::move(path), size);
  self._access_mode = AccessMode::WRITE;

  const int fd = shm_open(self._path.c_str(), O_RDWR | O_CREAT, 0666);
  if (fd == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::creation_error), error_category()));
  }

  if (ftruncate(fd, static_cast<__off_t>(self._size)) == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::resize_error), error_category()));
  }

  self._native_handle = fd;

  return self;
}

// _____________________________________________________________________________________________________________________
std::expected<shared_memory_file, std::error_code> shared_memory_file::open(std::string&& path, const AccessMode mode) {
  const int o_flags = (mode == AccessMode::WRITE) ? O_RDWR : O_RDONLY;

  const int fd = shm_open(path.c_str(), o_flags, 0666);
  if (fd == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::open_error), error_category()));
  }

  struct stat shm_stat{};
  if (fstat(fd, &shm_stat) == -1) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::file_not_found), error_category()));
  }

  shared_memory_file self(std::move(path), shm_stat.st_size);
  self._access_mode = mode;

  self._native_handle = fd;

  return self;
}