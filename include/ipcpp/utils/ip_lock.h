#pragma once

#include <memory>
#include <system_error>
#include <string>

#include <ipcpp/utils/platform.h>

#ifdef IPCPP_WINDOWS
#define NOMINMAX
#include <windows.h>
#include <system_error>
#else
#include <fcntl.h>
#include <unistd.h>
#include <system_error>
#endif

namespace ipcpp::utils {

class InterProcessLock {
 public:
  explicit InterProcessLock(const std::string& name);
  ~InterProcessLock();

  InterProcessLock(InterProcessLock&&) noexcept = default;
  InterProcessLock& operator=(InterProcessLock&&) noexcept = default;

  std::error_code lock();

  std::error_code try_lock(bool& acquired);

  void unlock();

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};

#ifdef IPCPP_WINDOWS

struct InterProcessLock::Impl {
  HANDLE mutex = nullptr;
};

InterProcessLock::InterProcessLock(const std::string& name)
    : _impl(std::make_unique<Impl>())
{
  _impl->mutex = CreateMutexA(
      nullptr,
      FALSE,
      name.c_str()
  );
}

std::error_code InterProcessLock::lock()
{
  DWORD rc = WaitForSingleObject(_impl->mutex, INFINITE);

  if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED) {
    // WAIT_ABANDONED = previous owner crashed
    return {};
  }

  return std::error_code(GetLastError(), std::system_category());
}

std::error_code InterProcessLock::try_lock(bool& acquired)
{
  DWORD rc = WaitForSingleObject(_impl->mutex, 0);

  if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED) {
    acquired = true;
    return {};
  }

  if (rc == WAIT_TIMEOUT) {
    acquired = false;
    return {};
  }

  return std::error_code(GetLastError(), std::system_category());
}

void InterProcessLock::unlock()
{
  ReleaseMutex(_impl->mutex);
}

InterProcessLock::~InterProcessLock()
{
  if (_impl && _impl->mutex) {
    CloseHandle(_impl->mutex);
  }
}

#else

struct InterProcessLock::Impl {
  explicit Impl(const std::string& name) : _fd(-1) {
    std::string path = std::format("{}/{}", std::filesystem::temp_directory_path().string(), name);
    _fd = open(path.c_str(), O_CREAT | O_RDWR, 0644);
    if (_fd == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to open file");
    }
  }

  Impl(Impl&& other) {
    _fd = other._fd;
    other._fd = -1;
  }

  ~Impl() {
    if (_fd != -1) {
      close(_fd);
    }
  }

  // Exclusive lock (write lock)
  std::error_code lock() const {
    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(_fd, F_SETLKW, &fl) == -1) {
      return std::error_code(errno, std::system_category());
    }
    return {};
  }

  // Unlock the file
  void unlock() const {
    struct flock fl = {};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(_fd, F_SETLK, &fl) == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to unlock file");
    }
  }

  // Try to acquire an exclusive lock (write lock) without blocking
  [[nodiscard]] std::error_code try_lock(bool& acquired) const {
    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(_fd, F_SETLK, &fl) == -1) {
      acquired = false;
      if (errno == EAGAIN || errno == EACCES) {
        return {};
      }
      return std::error_code(errno, std::system_category());
    }
    acquired = true;
    return {};
  }

 private:
  int _fd;
};

InterProcessLock::InterProcessLock(const std::string& name)
    : _impl(std::make_unique<Impl>(name))
{}

std::error_code InterProcessLock::lock()
{
  return _impl->lock();
}

std::error_code InterProcessLock::try_lock(bool& acquired)
{
  return _impl->try_lock(acquired);
}

void InterProcessLock::unlock()
{
  _impl->unlock();
}

InterProcessLock::~InterProcessLock() = default;

#endif

}