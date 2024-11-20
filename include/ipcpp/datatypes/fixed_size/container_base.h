/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace ipcpp::fixed_size {

template <std::size_t _N, typename _Tp>
class container_base {
 public:
  typedef _Tp value_type;

  public:
   container_base() = default;
   container_base(value_type init_value) {
     std::memset(_data, init_value, _N);
   }

   container_base(container_base&& other) {
     std::memcpy(_data, other._data, _N);
   }

   container_base(const container_base& other) {
     std::memcpy(_data, other._data, _N);
   }

   ~container_base();

   value_type* data() { return _data; }
   const value_type* data() const { return _data; }

   std::size_t size() const { return _N; }

  protected:
   value_type _data[_N]{};
};

}