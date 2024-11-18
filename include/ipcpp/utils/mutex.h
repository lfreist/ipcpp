/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <atomic>
#include <thread>

namespace ipcpp {

/**
 * @brief Simple mutex using std::atomic_flag.
 *
 * @remark implementation adopted from https://en.cppreference.com/w/cpp/atomic/atomic_flag
 */
class Mutex {
 public:
  Mutex() : _flag(0) {}

  void lock() noexcept {
    auto value = _flag.test_and_set(std::memory_order_acquire);
    while (value) {
      _flag.wait(true, std::memory_order_relaxed);
      value = _flag.test_and_set(std::memory_order_acquire);
    }
  }

  void unlock() noexcept {
    _flag.clear(std::memory_order_release);
    _flag.notify_one();
  }

 private:
  std::atomic_flag _flag;
};

class SharedMutex {
 public:
  void lock() noexcept {
    while (true) {
      _mutex.lock();
      int num_readers = _readers.load(std::memory_order_acquire);
      if (num_readers == 0) {
        break;
      }
      _mutex.unlock();
      _readers.wait(num_readers);
    }
  }

  void unlock() noexcept { _mutex.unlock(); }

  void shared_lock() noexcept {
    _mutex.lock();
    _readers.fetch_add(1, std::memory_order_acq_rel);
    _mutex.unlock();
  }

  void shared_unlock() noexcept {
    _readers.fetch_sub(1, std::memory_order_acq_rel);
    _readers.notify_all();
  }

 private:
  std::atomic_int _readers{0};
  Mutex _mutex;
};

/**
 * @brief RAII locking of Mutex. Similar to std::unique_lock but only provides basic locking and unlocking.
 */
template <typename MUTEX>
// requires (std::is_same_v<MUTEX, Mutex>() || std::is_same_v<MUTEX, SharedMutex>())
class UniqueLock {
 public:
  explicit UniqueLock(MUTEX& mutex) : _mutex(mutex) { lock(); }

  UniqueLock(const UniqueLock&) = delete;
  UniqueLock& operator=(const UniqueLock&) = delete;

  ~UniqueLock() { unlock(); }

  void lock() { _mutex.lock(); }

  void unlock() { _mutex.unlock(); }

 private:
  MUTEX& _mutex;
};

/**
 * @brief RAII shared locking of SharedMutex. Similar to std::shared_lock but much simpler with less functionality etc.
 */
class SharedLock {
 public:
  explicit SharedLock(SharedMutex& mutex) : _mutex(mutex) { _mutex.shared_lock(); }

  SharedLock(const SharedLock&) = delete;
  SharedLock& operator=(const SharedLock&) = delete;

  ~SharedLock() { _mutex.shared_unlock(); }

 private:
  SharedMutex& _mutex;
};

}  // namespace ipcpp
