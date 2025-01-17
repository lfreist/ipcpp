/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/concepts.h>
#include <ipcpp/utils/platform.h>

#include <atomic>
#include <concepts>
#include <mutex>
#include <shared_mutex>

namespace ipcpp {

// --- Forward Declarations --------------------------------------------------------------------------------------------
template <typename S, bool is_shared, bool is_const>
class LockedPtrAccess;

// --- Tag Types -------------------------------------------------------------------------------------------------------
struct ConstructWithMutex {};

/**
 * Synchronizes an object: Attaches a mutex to a given type and only allows locked access to this type.
 *
 * Adopted from [folly::synchronized](https://github.com/facebook/folly/blob/main/folly/synchronized.h).
 *  Only implements most basic functionality of folly::synchronized
 *
 * @tparam T - Thy underlying type
 * @tparam M - A Lockable type. Defaults to std::shared_mutex
 */
template <typename T, typename M = std::shared_mutex>
  requires concepts::lockable<M> && std::is_move_assignable_v<T> && std::is_move_constructible_v<T>
class synchronized {
  template <typename S, bool is_shared, bool is_const>
  friend class LockedPtrAccess;

 public:
  using value_type = std::remove_reference_t<T>;

 public:
  /// not copyable
  synchronized(const synchronized&) = delete;
  synchronized& operator=(const synchronized&) = delete;

  /// default movable
  synchronized(synchronized&&) noexcept = default;
  synchronized& operator=(synchronized&&) noexcept = default;

  /// default constructor: requires T to be default initializable
  synchronized()
    requires std::default_initializable<T>
  = default;
  ~synchronized() = default;

  /// Constructor for not copy and not move (Arg must not be type synchronized)
  template <typename Arg, typename... Args>
    requires(!std::same_as<std::remove_cvref_t<Arg>, synchronized>)
  explicit(sizeof...(Args) == 0) synchronized(Arg&& arg, Args&&... args)
      : _data(std::forward<Arg>(arg), std::forward<Args>(args)...), _mutex_like() {}

  /// Constructor accepting a lockable/mutex_like type (c.f. concepts.h)
  template <typename... Args>
  synchronized(ConstructWithMutex _, M mutex, Args&&... args)
      : _data(std::forward<Args>(args)...), _mutex_like(mutex) {}

  /**
   * Obtain a lock and then invoke the given function f with _data as reference parameter
   *
   * @tparam F - invocable type with T& as argument type
   * @param f - invocable object. is invoked using f(_data)
   * @return return type of f
   */
  template <typename F>
    requires std::is_invocable_v<F, T&>
  std::invoke_result_t<F, T&> with_write_lock(F f) {
    std::lock_guard lock(_mutex_like);
    return f(_data);
  }

  /**
   * const override of with_lock_write(F)
   *
   * @tparam F - invocable type with T& as argument type
   * @param f - invocable object. Invoked using f(_data)
   * @return return type of f
   */
  template <typename F>
    requires std::is_invocable_v<F, T&>
  std::invoke_result_t<F, T&> with_write_lock(F f) const {
    std::lock_guard lock(mutex());
    return f(_data);
  }

  /**
   * Obtain a shared lock and then invoke the given function f with _data as const reference argument
   *
   * @tparam F - invocable type with const T& as argument type
   * @param f - invocable object. Invoked using f(_data)
   * @return return type of f
   */
  template <typename F>
    requires std::is_invocable_v<F, T&> && concepts::SharedLockable<M>
  std::invoke_result_t<F, const T&> with_read_lock(F f) const {
    std::shared_lock lock(mutex());
    return f(_data);
  }

  /**
   * Obtain locked pointer like access to underlying data.
   *  Lock is released on destruction of returned value.
   *  Returned value can be treated as T*.
   *
   * ! Obtaining the same LockedPtrAccess a second time without destructing the first obtained results in a deadlock !
   *
   * @return LockedPtrAccess<T>
   */
  LockedPtrAccess<synchronized, false, false> wlock() { return LockedPtrAccess<synchronized, false, false>(this); }

  /**
   * Obtain const locked pointer like access to underlying data.
   *  Lock is released on destruction of returned value.
   *  Returned value can be treated as const T*.
   *
   * ! Obtaining the same LockedPtrAccess a second time without destructing the first obtained results in a deadlock !
   *
   * @return
   */
  LockedPtrAccess<synchronized, false, true> wlock() const { return LockedPtrAccess<synchronized, false, true>(this); }

  /**
   * Obtain locked const pointer like access to underlying data.
   *  Lock is released on destruction of returned value.
   *  Returned value can be treated as T* const.
   *
   * ! Obtaining the same LockedPtrAccess a second time without destructing the first obtained results in a deadlock !
   *
   * @tparam M_
   * @return
   */
  template <typename M_ = M>
    requires concepts::SharedLockable<M_>
  LockedPtrAccess<synchronized, true, true> rlock() const {
    return LockedPtrAccess<synchronized, true, true>(this);
  }

 private:
  M& mutex() const { return const_cast<M&>(_mutex_like); }

  T _data;
  M _mutex_like;
};

/**
 * Pointer like thread-safe access to some data.
 *  A mutex gets locked on construction and unlocked on destruction.
 *
 * @tparam S - synchronized type
 * @tparam is_shared_lock [bool] - indicated shared lock capabilities of underlying mutex
 * @tparam is_const [bool] - indicates const qualification of underlying type
 */
template <typename S, bool is_shared_lock, bool is_const>
class LockedPtrAccess {
  template <typename T, typename M>
    requires concepts::lockable<M> && std::is_move_assignable_v<T> && std::is_move_constructible_v<T>
  friend class synchronized;

  using value_type = typename S::value_type;
  using ptr_type = std::conditional_t<is_const, const S* const, S* const>;

 private:
  /**
   * Private Constructor: only synchronized is allowed to construct a LockedPtrAccess
   *
   * @param s
   */
  explicit LockedPtrAccess(ptr_type s)
    requires(is_const || !is_shared_lock)
      : _s(s) {
    if constexpr (is_shared_lock) {
      _s->mutex().lock_shared();
    } else {
      _s->mutex().lock();
    }
  }

 public:
  /// unlock on destruction
  ~LockedPtrAccess() {
    if constexpr (is_shared_lock) {
      _s->mutex().unlock_shared();
    } else {
      _s->mutex().unlock();
    }
  }

  /// Non const access to underlying data. Only allowed for exclusive non const locks.
  template <bool c = is_const>
    requires(!c)
  value_type& operator*() {
    return _s->_data;
  }

  /// Const access to underlying data
  const value_type& operator*() const { return _s->_data; }

  /// Non const access to underlying data. Only allowed for exclusive non const locks.
  template <bool c = is_const>
    requires(!c)
  value_type* operator->() {
    return &_s->_data;
  }

  /// Const access to underlying data.
  const value_type* operator->() const { return &_s->_data; }

 private:
  ptr_type _s;
};

}  // namespace ipcpp