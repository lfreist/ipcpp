/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <memory>

namespace ipcpp {

template <typename T_p>
class optional {
 public:
  optional() noexcept {};
  explicit optional(std::nullopt_t) noexcept {};

  optional(const optional& other) : _has_value(other._has_value) { emplace(other.value()); }
  optional(optional&& other) noexcept : _has_value(other._has_value) { emplace(std::move(other.value())); }

  template <typename... T_Args>
  explicit(sizeof...(T_Args) == 1) optional(T_Args&&... args) : _has_value(true) {
    std::construct_at(_m_ptr(), std::forward<T_Args&&>(args)...);
  }

  ~optional() { reset(); }

  void reset() {
    if constexpr (!std::is_trivially_destructible_v<T_p>) {
      if (_has_value) {
        std::destroy_at(_m_ptr());
        _has_value = false;
      }
    }
  }

  template <typename... T_Args>
  T_p& emplace(T_Args&&... args) {
    reset();
    if constexpr (std::is_trivially_default_constructible_v<T_p> && sizeof...(T_Args) == 0) {
      _has_value = true;
    } else {
      std::construct_at(_m_ptr(), std::forward<T_Args>(args)...);
      _has_value = true;
    }
    return value();
  }

  bool has_value() { return _has_value; }
  operator bool() { return _has_value; }

  T_p& value() { return *_m_ptr(); }
  const T_p& value() const { return *_m_ptr(); }

  T_p& operator*() { return *_m_ptr(); }
  const T_p& operator*() const { return *_m_ptr(); }

  T_p* operator->() { return _m_ptr(); }
  const T_p* operator->() const { return _m_ptr(); }

 private:
  T_p* _m_ptr() { return reinterpret_cast<T_p*>(std::addressof(_storage)); }
  const T_p* _m_ptr() const { return reinterpret_cast<const T_p*>(std::addressof(_storage)); }

 private:
  bool _has_value = false;
  alignas(T_p) char _storage[sizeof(T_p)];
};

}  // namespace ipcpp