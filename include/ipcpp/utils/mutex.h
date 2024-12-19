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

#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include <cassert>
#include <climits>
#include <cstdint>

namespace ipcpp {

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

  void lock() noexcept {
    while (_flag.exchange(true, std::memory_order_acquire)) {
      while (_flag.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
      }
    }
  }

  void unlock() noexcept {
    _flag.store(false, std::memory_order_release);
  }

  bool try_lock() noexcept { return !_flag.exchange(true, std::memory_order_acquire); }

  [[nodiscard]] bool is_locked() const noexcept { return _flag.load(std::memory_order_acquire); }

 private:
  alignas(std::hardware_destructive_interference_size) std::atomic_bool _flag;
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

  bool try_lock() noexcept {
    if (_mutex.try_lock()) {
      if (_readers.load(std::memory_order_acquire) == 0) {
        return true;
      }
      _mutex.unlock();
    }
    return false;
  }

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
  alignas(std::hardware_destructive_interference_size) std::atomic_int _readers{0};
  mutex _mutex;
};

static_assert(concepts::shared_mutex<shared_mutex>,
              "ipcpp::shared_mutex does not fulfill the requirements of shared_mutex");

#ifdef __linux__

class futex {
public:
  futex() : _flag(UNLOCKED) {}

  futex(const futex&) = delete;
  futex& operator=(const futex&) = delete;

  void lock() noexcept {
    if (int expected = UNLOCKED; !_flag.compare_exchange_strong(expected, LOCKED, std::memory_order_acquire)) {
      _m_wait_for_lock();
    }
  }

  bool try_lock() noexcept {
    int expected = UNLOCKED;
    return _flag.compare_exchange_strong(expected, LOCKED, std::memory_order_acquire);
  }

  void unlock() noexcept {
    _flag.store(UNLOCKED, std::memory_order_release);
    syscall(SYS_futex, &_flag, FUTEX_WAKE, 1, nullptr, nullptr, 0);
  }

private:
  alignas(std::hardware_destructive_interference_size) std::atomic<int32_t> _flag;

  static constexpr int32_t UNLOCKED = 0;
  static constexpr int32_t LOCKED = 1;

  void _m_wait_for_lock() noexcept {
    int expected = LOCKED;
    while (true) {
      if (_flag.load(std::memory_order_relaxed) == UNLOCKED) {
        if (_flag.compare_exchange_strong(expected, LOCKED, std::memory_order_acquire)) {
          return;
        }
      }
      syscall(SYS_futex, &_flag, FUTEX_WAIT, LOCKED, nullptr, nullptr, 0);
      expected = LOCKED;
    }
  }
};


class shared_futex {
 public:
  shared_futex() : _state(0) {}

  shared_futex(const shared_futex&) = delete;
  shared_futex& operator=(const shared_futex&) = delete;

  void lock_shared() noexcept {
    while (true) {
      if (int expected = _state.load(std::memory_order_acquire); expected >= 0) {
        if (_state.compare_exchange_weak(expected, expected + 1, std::memory_order_acquire)) {
          return;
        }
      } else {
        syscall(SYS_futex, &_state, FUTEX_WAIT, expected, nullptr, nullptr, 0);
      }
    }
  }

  void unlock_shared() noexcept {
    assert(_state.load(std::memory_order_relaxed) > 0);
    _state.fetch_sub(1, std::memory_order_release);

    if (_state.load(std::memory_order_relaxed) == 0) {
      syscall(SYS_futex, &_state, FUTEX_WAKE, 1, nullptr, nullptr, 0);
    }
  }

  void lock() noexcept {
    int expected = 0;
    while (!_state.compare_exchange_weak(expected, -1, std::memory_order_acquire)) {
      syscall(SYS_futex, &_state, FUTEX_WAIT, expected, nullptr, nullptr, 0);
      expected = 0;
    }
  }

  // Release exclusive (writer) lock
  void unlock() noexcept {
    assert(_state.load(std::memory_order_relaxed) == -1);
    _state.store(0, std::memory_order_release);

    syscall(SYS_futex, &_state, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
  }

 private:
  /// -1: exclusive lock, >0: shared locks, 0: free
  alignas(std::hardware_destructive_interference_size) std::atomic<int32_t> _state;
};

#endif

}  // namespace ipcpp
