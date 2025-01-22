/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <concepts>

namespace carry {

template <typename T_Iter>
concept forward_iterator = requires(T_Iter it) {
  { it++ } -> std::convertible_to<T_Iter>;
  { ++it } -> std::same_as<T_Iter&>;
  { *it } -> std::convertible_to<typename T_Iter::value_type>;
} && requires { typename T_Iter::value_type; };

}  // namespace carry