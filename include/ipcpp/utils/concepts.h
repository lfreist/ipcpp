/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <concepts>

namespace ipcpp::utils::concepts {

template <typename T>
concept Lockable = requires(T t) {
  { t.lock() } -> std::same_as<void>;
  { t.unlock() } -> std::same_as<void>;
};

template <typename T>
concept SharedLockable = requires(T t) {
  { t.lock_shared() } -> std::same_as<void>;
  { t.unlock_shared() } -> std::same_as<void>;
};

}