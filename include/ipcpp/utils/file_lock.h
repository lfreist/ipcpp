/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <filesystem>
#include <format>

namespace ipcpp {

class file_lock {
 public:
  explicit file_lock(const std::string& name) : filePath_(std::format("{}/{}", std::filesystem::temp_directory_path().string(), name)), fd_(-1) {
    // Open the file for locking
    fd_ = open(filePath_.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd_ == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to open file");
    }
  }

  ~file_lock() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  // Exclusive lock (write lock)
  void lock() const {
    struct flock fl = {};
    fl.l_type = F_WRLCK;  // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the whole file

    if (fcntl(fd_, F_SETLKW, &fl) == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to lock file");
    }
  }

  // Shared lock (read lock)
  void lock_shared() const {
    struct flock fl = {};
    fl.l_type = F_RDLCK;  // Read lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the whole file

    if (fcntl(fd_, F_SETLKW, &fl) == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to acquire shared lock on file");
    }
  }

  // Unlock the file
  void unlock() const {
    struct flock fl = {};
    fl.l_type = F_UNLCK;  // Unlock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Unlock the whole file

    if (fcntl(fd_, F_SETLK, &fl) == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to unlock file");
    }
  }

  // Unlock the file for shared locks
  void unlock_shared() const {
    unlock();  // Shared and exclusive locks are both released by the same unlock operation
  }

  // Try to acquire an exclusive lock (write lock) without blocking
  [[nodiscard]] bool try_lock() const {
    struct flock fl = {};
    fl.l_type = F_WRLCK;  // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the whole file

    if (fcntl(fd_, F_SETLK, &fl) == -1) {
      if (errno == EAGAIN || errno == EACCES) {
        return false;  // File is already locked
      }
      throw std::system_error(errno, std::generic_category(), "Failed to try_lock file");
    }
    return true;
  }

  // Try to acquire a shared lock (read lock) without blocking
  [[nodiscard]] bool try_lock_shared() const {
    struct flock fl = {};
    fl.l_type = F_RDLCK;  // Read lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the whole file

    if (fcntl(fd_, F_SETLK, &fl) == -1) {
      if (errno == EAGAIN || errno == EACCES) {
        return false;  // File is already locked
      }
      throw std::system_error(errno, std::generic_category(), "Failed to try_lock_shared file");
    }
    return true;
  }

 private:
  std::string filePath_;
  int fd_;
};

class unique_file_lock {
 public:
  explicit unique_file_lock(file_lock& handle) : _handle(handle) { _handle.lock(); }

  ~unique_file_lock() { _handle.unlock(); }

  void lock() const {
    _handle.lock();
  }

  void unlock() const {
    _handle.unlock();
  }

  [[nodiscard]] bool try_lock() const {
    return _handle.try_lock();
  }

 private:
  file_lock& _handle;
};

class shared_file_lock {
 public:
  explicit shared_file_lock(file_lock& handle) : _handle(handle) { _handle.lock_shared(); }

  ~shared_file_lock() { _handle.unlock_shared(); }

  void lock() const {
    _handle.lock();
  }

  void lock_shared() const {
    _handle.lock_shared();
  }

  void unlock() const {
    _handle.unlock();
  }

  [[nodiscard]] bool try_lock() const {
    return _handle.try_lock();
  }

  [[nodiscard]] bool try_lock_shared() const {
    return _handle.try_lock_shared();
  }

 private:
  file_lock& _handle;
};

}  // namespace ipcpp