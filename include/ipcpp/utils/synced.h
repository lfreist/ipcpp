/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <optional>

#include <ipcpp/utils/mutex.h>

namespace ipcpp {

template <typename T_p>
class synced {
 public:
  template <bool is_shared, bool is_const>
  requires requires() {
      // access must not be shared and for write!
      !(is_shared && !is_const);
    }
  class access_wrapper {
    typedef std::conditional_t<is_const, const T_p, T_p> data_type;
   public:
    access_wrapper(T_p* data, shared_mutex* mutex) : _shared_mutex(mutex), _data(data) {}

    access_wrapper(access_wrapper&& other) noexcept {
      std::swap(other._data, _data);
      std::swap(other._shared_mutex, _shared_mutex);
    }

    access_wrapper(const access_wrapper&) = delete;

    ~access_wrapper() {
      if (_shared_mutex != nullptr) {
        if constexpr (is_shared) {
          _shared_mutex->unlock_shared();
        } else {
          _shared_mutex->unlock();
        }
      }
    }

    data_type* operator->() {
      return _data;
    }

    data_type& operator*() {
      return *_data;
    }

   private:
    shared_mutex* _shared_mutex = nullptr;
    data_type* _data = nullptr;
  };

 public:
  explicit synced(T_p&& value) requires std::is_move_constructible_v<T_p> : _data(std::move(value)) {}

  template <typename... T_Args>
  requires std::is_constructible_v<T_p, T_Args...>
  explicit(sizeof...(T_Args) == 1) synced(T_Args&&... args) : _data(std::forward<T_Args>(args)...) {}

 public:
  std::optional<access_wrapper<true, true>> shared_read_access(int num_retries) {
    if (_shared_mutex.try_lock_shared(num_retries)) {
      return access_wrapper<true, true>(&_data, &_shared_mutex);
    }
    return std::nullopt;
  }

  std::optional<access_wrapper<false, true>> unique_read_access() {
    if (_shared_mutex.try_lock()) {
      return access_wrapper<false, true>(&_data, &_shared_mutex);
    }
    return std::nullopt;
  }

  std::optional<access_wrapper<false, false>> write_access() {
    if (_shared_mutex.try_lock()) {
      return access_wrapper<false, false>(&_data, &_shared_mutex);
    }
    return std::nullopt;
  }

 private:
  shared_mutex _shared_mutex;
  T_p _data;
};

}