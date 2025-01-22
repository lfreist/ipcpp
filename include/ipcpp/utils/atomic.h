/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <atomic>
#include <concepts>

namespace carry::atomic {

template <typename... Ts>
struct largest_lock_free;

template <typename T, typename... Ts>
struct largest_lock_free<T, Ts...> {
  using type = std::conditional_t<
      std::atomic<T>::is_always_lock_free,
      T,
      typename largest_lock_free<Ts...>::type>;
};

template <typename T, typename... Ts>
using largest_lock_free_t = largest_lock_free<T, Ts...>::type;

template <>
struct largest_lock_free<> {
  using type = void;
};

// Usage
using largest_lock_free_uint_t = largest_lock_free_t<
    std::uint64_t, std::uint32_t, std::uint16_t, std::uint8_t>;

}