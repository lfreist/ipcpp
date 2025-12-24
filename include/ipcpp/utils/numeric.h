/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cassert>
#include <concepts>
#include <limits>
#include <cstdint>

namespace ipcpp::numeric {

template <typename T_p>
  requires std::is_unsigned_v<T_p> && std::is_integral_v<T_p>
inline constexpr int count_leading_zero(T_p v) {
#if defined(__GNUC__) || defined(__clang__)
  if constexpr (std::is_same_v<T_p, unsigned int>) {
    return __builtin_clz(v);
  } else if constexpr (std::is_same_v<T_p, unsigned long>) {
    return __builtin_clzl(v);
  } else if constexpr (std::is_same_v<T_p, unsigned long long>) {
    return __builtin_clzll(v);
  }
#elif defined(_MSC_VER)
  if constexpr (sizeof(T_p) <= sizeof(unsigned int)) {
    unsigned long leading_zero_count;
    _BitScanReverse(&leading_zero_count, static_cast<unsigned int>(v));
    return std::numeric_limits<unsigned int>::digits - 1 - leading_zero_count;
  } else if constexpr (sizeof(T_p) <= sizeof(unsigned long)) {
    unsigned long leading_zero_count;
    _BitScanReverse(&leading_zero_count, static_cast<unsigned long>(v));
    return std::numeric_limits<unsigned long>::digits - 1 - leading_zero_count;
  } else if constexpr (sizeof(T_p) <= sizeof(unsigned long long)) {
    unsigned long leading_zero_count;
    _BitScanReverse64(&leading_zero_count, static_cast<unsigned long long>(v));
    return std::numeric_limits<unsigned long long>::digits - 1 - leading_zero_count;
  }
#else
  // Fallback for platforms without compiler intrinsics
  if (v == 0) {
    return std::numeric_limits<T_p>::digits;
  }

  int count = 0;
  constexpr int total_bits = std::numeric_limits<T_p>::digits;
  T_p mask = T_p(1) << (total_bits - 1);
  while ((v & mask) == 0) {
    ++count;
    mask >>= 1;
  }
  return count;
#endif
}

/**
 * Checks whether a given integral type (T_p) is a power of two.
 *
 */
template <typename T_p>
  requires std::is_unsigned_v<T_p> && std::is_integral_v<T_p>
inline constexpr bool is_power_of_two(T_p v) {
  return (v & (v - 1)) == T_p(0);
}

/**
 * Returns the largest power of two that is smaller or equal to v.
 *  e.g. floor_to_power_of_two(v) is a power of 2 AND <= v
 *
 */
template <typename T_p>
  requires std::is_unsigned_v<T_p> && std::is_integral_v<T_p>
inline constexpr T_p floor_to_power_of_two(T_p v) {
  if (is_power_of_two(v)) {
    return v;
  }
  return T_p(1) << (std::numeric_limits<T_p>::digits - count_leading_zero(v) - 1);
}

template <typename T_p>
  requires std::is_unsigned_v<T_p> && std::is_integral_v<T_p>
inline constexpr T_p ceil_to_power_of_two(T_p v) {
  if (is_power_of_two(v)) {
    return v;
  }
  return T_p(1) << (std::numeric_limits<T_p>::digits - count_leading_zero(v));
}


template <typename T>
struct half_size_int;

template <>
struct half_size_int<uint64_t> {
  using type = uint32_t;
};

template <>
struct half_size_int<uint32_t> {
  using type = uint16_t;
};

template <>
struct half_size_int<uint16_t> {
  using type = uint8_t;
};

template <>
struct half_size_int<int64_t> {
  using type = int32_t;
};

template <>
struct half_size_int<int32_t> {
  using type = int16_t;
};

template <>
struct half_size_int<int16_t> {
  using type = int8_t;
};


template <typename T>
using half_size_int_t = typename half_size_int<T>::type;

}  // namespace ipcpp::numeric