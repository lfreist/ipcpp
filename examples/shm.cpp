//
// Created by lfreist on 14/10/2024.
//

#pragma once
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <span>
#include <functional>
#include <cstddef>
#include <stdexcept>
#include <cstring>
#include <expected>
#include <cstdint>
#include <chrono>
#include <iostream>

#define TO_STRING(a) #a

enum class LockError {
  ALREADY_LOCKED_FOR_READ,
  ALREADY_LOCKED_FOR_WRITE,
  READ_LOCK_TIMEOUT,
  WRITE_LOCK_TIMEOUT,
  UNKNOWN_ACCESS_TYPE,
  UNKNOWN_LOCK_ERROR,
};

std::ostream& operator<<(std::ostream& os, LockError error) {
  switch (error) {
    case LockError::ALREADY_LOCKED_FOR_READ:
      os << TO_STRING(LockError::ALREADY_LOCKED_FOR_READ);
      break;
    case LockError::ALREADY_LOCKED_FOR_WRITE:
      os << TO_STRING(LockError::ALREADY_LOCKED_FOR_WRITE);
      break;
    case LockError::READ_LOCK_TIMEOUT:
      os << TO_STRING(LockError::READ_LOCK_TIMEOUT);
      break;
    case LockError::WRITE_LOCK_TIMEOUT:
      os << TO_STRING(LockError::WRITE_LOCK_TIMEOUT);
      break;
    case LockError::UNKNOWN_ACCESS_TYPE:
      os << TO_STRING(LockError::UNKNOWN_ACCESS_TYPE);
      break;
    case LockError::UNKNOWN_LOCK_ERROR:
      os << TO_STRING(LockError::UNKNOWN_LOCK_ERROR);
      break;
  }
  return os;
}

class SharedMemory {
 public:
  enum class AccessT {
    READ,
    WRITE,
  };

  template <AccessT AT>
  class SynchronizedSharedData {
   public:
    ~SynchronizedSharedData() {
      if (_rwlock) {
        pthread_rwlock_unlock(_rwlock);
      }
    }

    static std::expected<SynchronizedSharedData<AT>, LockError> init(void* start, std::size_t size, pthread_rwlock_t* rwlock) {
      SynchronizedSharedData<AT> self(start, size);
      self._rwlock = rwlock;

      if constexpr (AT == AccessT::READ) {
        if (pthread_rwlock_rdlock(self._rwlock) != 0) {
          return std::unexpected(LockError::ALREADY_LOCKED_FOR_READ);
        }
      } else if constexpr (AT == AccessT::WRITE) {
        if (pthread_rwlock_wrlock(self._rwlock) != 0) {
          return std::unexpected(LockError::ALREADY_LOCKED_FOR_WRITE);
        }
      } else {
        return std::unexpected(LockError::UNKNOWN_ACCESS_TYPE);
      }

      return self;
    }

    static std::expected<SynchronizedSharedData<AT>, LockError> init(void* start, std::size_t size, pthread_rwlock_t* rwlock, std::chrono::seconds timeout) {
      SynchronizedSharedData<AT> self(start, size);
      self._rwlock = rwlock;

      timespec t{};
      clock_gettime(CLOCK_REALTIME, &t);
      t.tv_sec += timeout.count();
      t.tv_nsec = 0;

      if constexpr (AT == AccessT::READ) {
        int res = pthread_rwlock_timedrdlock(self._rwlock, &t);
        if (res == ETIMEDOUT) {
          return std::unexpected(LockError::READ_LOCK_TIMEOUT);
        } else if (res != 0) {
          return std::unexpected(LockError::UNKNOWN_LOCK_ERROR);
        }
      } else if constexpr (AT == AccessT::WRITE) {
        int res = pthread_rwlock_timedwrlock(self._rwlock, &t);
        if (res == ETIMEDOUT) {
          return std::unexpected(LockError::WRITE_LOCK_TIMEOUT);
        } else if (res != 0) {
          return std::unexpected(LockError::UNKNOWN_LOCK_ERROR);
        }
      } else {
        return std::unexpected(LockError::UNKNOWN_ACCESS_TYPE);
      }

      return self;
    }

    template <typename T>
    std::span<T> data() { return std::span<T>((T*)_start, _size); }

    SynchronizedSharedData(SynchronizedSharedData&& other) noexcept {
      _start = other._start;
      _size = other._size;
      _rwlock = other._rwlock;
      other._start = nullptr;
      other._size = 0;
      other._rwlock = nullptr;
    }

    SynchronizedSharedData(const SynchronizedSharedData&) = delete;

   private:
    SynchronizedSharedData(void* start, std::size_t size) : _start(start), _size(size) {}

   private:
    void* _start = nullptr;
    std::size_t _size = 0;
    pthread_rwlock_t* _rwlock = nullptr;  // no ownership
  };
  SharedMemory(const char* path, std::size_t size);
  ~SharedMemory();

  enum class Sync { LOCKED, LOCK_FREE };

  template <typename T, typename F>
  void with_read(F& f, std::size_t start_offset, std::size_t size);

  template <typename T, typename F>
  void with_write(F&& f, std::size_t start_offset, std::size_t size);

  template <AccessT AT>
  std::expected<SynchronizedSharedData<AT>, LockError> acquire(std::chrono::seconds timeout) {
    return SynchronizedSharedData<AT>::init((void*)((uint8_t*)_start + sizeof (pthread_rwlock_t)), _size, _rwlock, timeout);
  }

  template <AccessT AT>
  std::expected<SynchronizedSharedData<AT>, LockError> acquire() {
    return SynchronizedSharedData<AT>::init((void*)((uint8_t*)_start + sizeof (pthread_rwlock_t)), _size, _rwlock);
  }

 private:
  void* _start = nullptr;
  std::size_t _size = 0;
  pthread_rwlock_t* _rwlock = nullptr;
};

SharedMemory::SharedMemory(const char* path, std::size_t size) : _size(size) {
  int fd = shm_open(path, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    throw std::runtime_error("Failed to open shared memory");
  }

  if (ftruncate(fd, size) == -1) {
    throw std::runtime_error("Failed to resize shared memory");
  }

  _start = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (_start == MAP_FAILED) {
    throw std::runtime_error("Failed to map shared memory");
  }

  close(fd);

  _rwlock = static_cast<pthread_rwlock_t*>(_start);

  bool need_init = false;
  if (pthread_rwlock_tryrdlock(_rwlock) != 0) {
    need_init = true;
  } else {
    pthread_rwlock_unlock(_rwlock);
  }

  if (need_init) {
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_rwlock_init(_rwlock, &attr) != 0) {
      throw std::runtime_error("Failed to initialize read-write lock");
    }
    pthread_rwlockattr_destroy(&attr);
  }
}

SharedMemory::~SharedMemory() {
  munmap(_start, _size);
}

template <typename T, typename F>
void SharedMemory::with_read(F& f, std::size_t start_offset, std::size_t size) {
  if (pthread_rwlock_rdlock(_rwlock) != 0) {
    throw std::runtime_error("Failed to acquire read lock");
  }

  T* read_start = static_cast<T*>(static_cast<void*>(static_cast<char*>(_start) + sizeof(pthread_rwlock_t) + start_offset));
  f(std::span<T>(read_start, size));

  if (pthread_rwlock_unlock(_rwlock) != 0) {
    throw std::runtime_error("Failed to release read lock");
  }
}

template <typename T, typename F>
void SharedMemory::with_write(F&& f, std::size_t start_offset, std::size_t size) {
  if (pthread_rwlock_wrlock(_rwlock) != 0) {
    throw std::runtime_error("Failed to acquire write lock");
  }

  T* write_start = static_cast<T*>(static_cast<void*>(static_cast<char*>(_start) + sizeof(pthread_rwlock_t) + start_offset));
  f(std::span<T>(write_start, size));

  if (pthread_rwlock_unlock(_rwlock) != 0) {
    throw std::runtime_error("Failed to release write lock");
  }
}
