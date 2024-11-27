/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <type_traits>
#include <ipcpp/shm/dynamic_allocator.h>

namespace ipcpp {

namespace detail {

template <typename, typename = std::void_t<>>
struct has_typedef : std::false_type {};

template <typename T>
struct has_typedef<T, std::void_t<T>> : std::true_type {};

template <typename T, typename Default>
using detect_or_t = std::conditional_t<has_typedef<T>::value, T, Default>;

}  // namespace detail

template <class T_Alloc>
struct allocator_traits {
  typedef T_Alloc allocator_type;
  typedef typename T_Alloc::value_type value_type;
  typedef typename T_Alloc::difference_type difference_type;
  typedef typename T_Alloc::size_type size_type;

  using alloc_return_type = detail::detect_or_t<typename T_Alloc::alloc_return_type, value_type*>;

 public:
  [[nodiscard]] static constexpr alloc_return_type allocate(T_Alloc& a, size_type n) {
    return a.allocate(n);
  }
  [[nodiscard]] static constexpr alloc_return_type allocate(size_type n) {}
};



}  // namespace ipcpp