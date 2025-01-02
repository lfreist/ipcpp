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
    assert(_flag.load(std::memory_order_acquire));
    _flag.store(false, std::memory_order_release);
  }

  bool try_lock() noexcept { return !_flag.exchange(true, std::memory_order_acquire); }
  bool try_lock(int retries) noexcept {
    if (!_flag.exchange(true, std::memory_order_acquire)) {
      return true;
    }
    for (; retries > 0; --retries) {
      if (!_flag.exchange(true, std::memory_order_acquire)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool is_locked() const noexcept { return _flag.load(std::memory_order_acquire); }

 private:
  alignas(std::hardware_destructive_interference_size) std::atomic_bool _flag;
};

static_assert(concepts::lockable<mutex>, "ipcpp::mutex does not fulfill the requirements of mutex");

class shared_mutex {
 public:
  void lock() noexcept {
    while (true) {
      std::int64_t expected = 0;
      if (_flag.compare_exchange_weak(expected, -1, std::memory_order_acq_rel)) {
        break;
      }
      std::this_thread::yield();
    }
  }

  void unlock() noexcept {
    assert(_flag.load(std::memory_order_acquire) == -1);
    _flag.store(0, std::memory_order_release);
  }

  bool try_lock() noexcept {
    std::int64_t expected = 0;
    return _flag.compare_exchange_weak(expected, -1, std::memory_order_acq_rel);
  }

  void lock_shared() noexcept {
    while (true) {
      std::int64_t expected = _flag.load(std::memory_order_acquire);
      if (expected == -1) {
        std::this_thread::yield();
        continue;
      }
      if (_flag.compare_exchange_weak(expected, expected + 1, std::memory_order_acq_rel)) {
        break;
      }
    }
  }

  void unlock_shared() noexcept {
    assert(_flag.load(std::memory_order_acquire) > 0);
    _flag.fetch_sub(1, std::memory_order_release);
  }

  bool try_lock_shared() noexcept {
    std::int64_t expected = _flag.load(std::memory_order_acquire);
    if (expected == -1) {
      return false;
    }
    return _flag.compare_exchange_weak(expected, expected + 1, std::memory_order_acq_rel);
  }

  [[nodiscard]] bool is_locked() const noexcept {
    return _flag.load(std::memory_order_acquire) == -1;
  }

  [[nodiscard]] bool is_locked_shared() const noexcept {
    return _flag.load(std::memory_order_acquire) > 0;
  }

  /**
   * @warning only use with care. we use it in single writer, multiple reader contexts to bypass the following issue:
   *  - reader 0 acquires lock
   *  - reader 1 acquires lock but between loading and writing the flag, reader 0 (or another reader) changes the value
   *    so that compare_exchange does not work anymore
   *  For our use case, we consider this a workaround: the time between reading and writing the value is quite short and
   *  retrying the process has very good chances to work as expected. However it is still possible, that it fails for
   *  every retry. In this case, we have a false negative shared lock!
   *
   *  When setting retries to ((max_num_subscribers * 2) + 1), at least one retry will work since there are only
   *   (max_num_subscribers * 2) reader side changes to _flag possible. While it is still possible, that in between the
   *   writer acquires an exclusive lock even though the reader tried to access it first, this case is considered an
   *   acceptable edge case because we dont give guarantees about the minimum lifetime of a shared chunk.
   *   If it is absolutely necessary that a reader reads a chunk, the writer should be configured in a way that it
   *   blocks on back pressure.
   */
  bool try_lock_shared(int retries) noexcept {
    for (int r = 0; r < (retries + 1); ++r) {
      std::int64_t expected = _flag.load(std::memory_order_acquire);
      if (expected == -1) {
        // for our needs, we can immediately return if a writer holds the lock
        return false;
      }
      if (_flag.compare_exchange_weak(expected, expected + 1, std::memory_order_acq_rel)) {
        return true;
      }
    }
    return false;
  }

 private:
  /// -1: read locked, 0: free, >0: write locked
  alignas(std::hardware_destructive_interference_size) std::atomic_int64_t _flag{0};
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
