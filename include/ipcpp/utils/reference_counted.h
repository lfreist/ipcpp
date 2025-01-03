/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/types.h>
#include <ipcpp/utils/mutex.h>

#include <cstdint>
#include <utility>

namespace ipcpp {

template <typename T_p>
class reference_counted {
 public:
  template <AccessMode T_AM = AccessMode::READ>
  class data_access {
   public:
    data_access(std::optional<T_p>& data, std::atomic_size_t& remaining_accesses)
        : _data(data), _remaining_access_ref(remaining_accesses) {
      assert(data.has_value());
    }
    ~data_access() {
      if (_remaining_access_ref.fetch_sub(1, std::memory_order_release) == 1) {
        _data.reset();
      }
    }

    T_p* operator->()
      requires(T_AM == AccessMode::WRITE)
    {
      return &_data.value();
    }
    const T_p* operator->() const { return &_data.value(); }

    T_p& operator*()
      requires(T_AM == AccessMode::WRITE)
    {
      return _data.value();
    }
    const T_p& operator*() const { return _data.value(); }

   private:
    std::optional<T_p>& _data;
    std::atomic_size_t& _remaining_access_ref;
  };

 public:
  reference_counted(T_p&& data, const std::size_t max_num_accesses)
      : _data(std::move(data)), _remaining_accesses(max_num_accesses) {}

  reference_counted(const T_p& data, const std::size_t max_num_accesses)
      : _data(data), _remaining_accesses(max_num_accesses) {}

  reference_counted() : _remaining_accesses(0) {}

  reference_counted(const reference_counted&) = delete;
  reference_counted& operator=(const reference_counted&) = delete;

  reference_counted(reference_counted&&) = delete;
  reference_counted& operator=(reference_counted&&) = delete;

  /**
   * @brief Calling acquire returns a data_access that may destroy _data on its destruction
   * @return
   */
  std::optional<data_access<AccessMode::READ>> consume() {
    if (_data.has_value()) {
      return data_access<AccessMode::READ>(_data, _remaining_accesses);
    }
    return std::nullopt;
  }

  void reset() { _data.reset(); }

  template <typename... T_Args>
  void emplace(std::size_t remaining_accesses, T_Args&&... args) {
    _data.reset();
    _remaining_accesses = remaining_accesses;
    _data = T_p(std::forward<T_Args>(args)...);
  }

  std::optional<T_p>& operator*() { return _data; }
  const std::optional<T_p>& operator*() const { return _data; }

  [[nodiscard]] std::size_t remaining_accesses() const { return _remaining_accesses.load(std::memory_order_acquire); }

 private:
  std::optional<T_p> _data;
  alignas(std::hardware_destructive_interference_size) std::atomic_size_t _remaining_accesses;
};

}  // namespace ipcpp