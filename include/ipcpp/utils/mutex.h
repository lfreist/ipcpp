/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/concepts.h>

#include <atomic>
#include <thread>
#include <mutex>

namespace ipcpp {

inline void wait(const std::atomic_flag& flag, const bool old, const std::memory_order order = std::memory_order_acquire) {
  while (flag.test(order) == old) {
    std::this_thread::yield();
  }
}

template <typename T> requires std::is_integral_v<T>
void wait(const std::atomic<T>& value, const T old, const std::memory_order order = std::memory_order_acquire) {
  while (value.load(order) == old) {
    std::this_thread::yield();
  }
}

/**
 * @brief mutex implementation according to std::mutex (except for mutex::is_locked which is added for checks in debug
 *  mode). Fulfills the requirements of a mutex according to the c++ standard.
 *
 * @remark spin-lock implementation using std::atomic_flag
 */
class mutex {
 public:
  mutex() : _flag(false) {}

  /// not copy-assignable
  mutex& operator=(mutex&) = delete;

  void lock() noexcept {
    auto value = _flag.test_and_set(std::memory_order_acquire);
    while (value) {
      wait(_flag, true, std::memory_order_relaxed);
      value = _flag.test_and_set(std::memory_order_acquire);
    }
  }

  void unlock() noexcept {
    _flag.clear(std::memory_order_release);
  }

  bool try_lock() noexcept { return _flag.test_and_set(std::memory_order_acquire); }

  [[nodiscard]] bool is_locked() const noexcept { return _flag.test(); }

 private:
  std::atomic_flag _flag;
};

static_assert(concepts::lockable<mutex>, "ipcpp::mutex does not fulfill the requirements of mutex");

class shared_mutex {
 public:
  void lock() noexcept {
    while (true) {
      _mutex.lock();
      const int num_readers = _readers.load(std::memory_order_acquire);
      if (num_readers == 0) {
        break;
      }
      _mutex.unlock();
      wait(_readers, num_readers, std::memory_order_acquire);
    }
  }

  void unlock() noexcept { _mutex.unlock(); }

  bool try_lock() noexcept { return _mutex.try_lock(); }

  void lock_shared() noexcept {
    _mutex.lock();
    _readers.fetch_add(1, std::memory_order_acq_rel);
    _mutex.unlock();
  }

  void unlock_shared() noexcept {
    _readers.fetch_sub(1, std::memory_order_acq_rel);
  }

  bool try_lock_shared() noexcept {
    if (_mutex.try_lock()) {
      _readers.fetch_add(1, std::memory_order_acq_rel);
      _mutex.unlock();
      return true;
    }
    return false;
  }

  [[nodiscard]] bool is_locked() const noexcept {
    return _mutex.is_locked();
  }

  [[nodiscard]] bool is_locked_shared() const noexcept {
    return _readers.load() > 0;
  }

 private:
  std::atomic_int _readers{0};
  mutex _mutex;
};

static_assert(concepts::shared_mutex<shared_mutex>,
              "ipcpp::shared_mutex does not fulfill the requirements of shared_mutex");


}  // namespace ipcpp
