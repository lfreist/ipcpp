/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/mutex.h>

#include <cstdint>

namespace ipcpp {

template <typename T_p>
class reference_counted {
 public:
  typedef T_p value_type;
  typedef T_p* pointer;
  typedef T_p& reference;

  class data_access {
   public:
    data_access(reference data, std::atomic_size_t& remaining_accesses) : _data(data), _remaining_access_ref(remaining_accesses) {}
    ~data_access() {
      if (_remaining_access_ref.fetch_sub(1) == 1) {
        std::destroy_at(std::addressof(_data));
      }
    }

    value_type* operator ->() {
      return &_data;
    }

    const value_type* operator->() const {
      return &_data;
    }

    value_type& operator*() { return _data; }
    const value_type& operator*() const { return _data; }

   private:
    value_type& _data;
    std::atomic_size_t& _remaining_access_ref;
  };

 public:
  reference_counted(T_p&& data, std::size_t max_num_accesses)
      : _data(std::move(data)), _remaining_accesses(max_num_accesses) {}

  reference_counted(const T_p& data, std::size_t max_num_accesses)
      : _data(data), _remaining_accesses(max_num_accesses) {}

  reference_counted(const reference_counted&) = delete;
  reference_counted& operator=(const reference_counted&) = delete;

  reference_counted(reference_counted&&) = delete;
  reference_counted& operator=(reference_counted&&) = delete;

  /**
   * @brief Calling consume returns a data_access that may destroy _data on its destruction
   * @return
   */
  data_access consume() {
    return data_access(_data, _remaining_accesses);
  }

 private:
  T_p _data;
  std::atomic_size_t _remaining_accesses;
};

}  // namespace ipcpp