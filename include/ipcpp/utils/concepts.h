/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <concepts>

namespace carry::concepts {

template <typename T>
concept movable = requires { requires std::is_move_constructible_v<T> || std::is_move_assignable_v<T>; };

template <typename T>
concept copyable = requires { requires std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>; };

template <typename T>
concept basic_lockable = requires(T t) {
  { t.lock() } -> std::same_as<void>;
  { t.unlock() } -> std::same_as<void>;
};

template <typename T>
concept lockable = requires(T t) {
  { t.try_lock() } -> std::same_as<bool>;
} && requires { requires basic_lockable<T>; };

template <typename T>
concept basic_shared_lockable = requires(T t) {
  { t.lock_shared() } -> std::same_as<void>;
  { t.unlock_shared() } -> std::same_as<void>;
} && requires { requires basic_lockable<T>; };

template <typename T>
concept shared_lockable = requires(T t) {
  { t.try_lock_shared() } -> std::same_as<bool>;
} && requires { requires lockable<T> && basic_shared_lockable<T>; };

template <typename T>
concept mutex = requires {
  requires lockable<T> && std::is_default_constructible_v<T> && std::is_destructible_v<T> && !movable<T> &&
               !copyable<T>;
};

template <typename T>
concept shared_mutex = requires { requires mutex<T> && shared_lockable<T>; };

template <typename T>
concept SharedLockable = requires(T t) {
  { t.lock_shared() } -> std::same_as<void>;
  { t.unlock_shared() } -> std::same_as<void>;
};

}  // namespace carry::concepts