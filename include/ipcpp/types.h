//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <ipcpp/shm/error.h>

#include <chrono>
#include <concepts>
#include <expected>
#include <ostream>
#include <functional>

#define AS_STRING(x) #x

namespace ipcpp {

enum class AccessMode {
  READ,
  WRITE,
};

std::ostream& operator<<(std::ostream& os, AccessMode am) {
  switch (am) {
    case AccessMode::READ:
      os << AS_STRING(AccessMode::READ);
      break;
    case AccessMode::WRITE:
      os << AS_STRING(AccessMode::WRITE);
      break;
  }
  return os;
}


namespace notification {

enum class NotificationType {
  UNINITIALIZED,
  REGISTRATION_SUCCESS,
  REGISTRATION_FAILED,
  PUBLISHER_DOWN,
  REGULAR,
};

}

// === concepts ========================================================================================================
namespace concepts {
template <typename T>
concept Duration = std::is_same_v<T, std::chrono::duration<typename T::rep, typename T::period>>;

/**
 * SynchronizedData must assure that reading and/or writing data is okay while the object lives
 * @tparam T
 */
template <typename T, template <AccessMode> typename View>
concept SynchronizedData = requires(T t, void* addr, std::chrono::nanoseconds duration) {
  { t.create(addr) } -> std::same_as<std::expected<T, shm::error::AccessError>>;
  { t.mem_size() } -> std::same_as<std::size_t>;
  { t.read_view() } -> std::same_as<View<AccessMode::READ>>;
  { t.write_view() } -> std::same_as<View<AccessMode::WRITE>>;
};

template <typename T, typename E, typename... Args>
concept HasCreate = requires(Args&&... args) {
  { T::create(std::forward<Args>(args)...) } -> std::same_as<std::expected<T, E>>;
};

template <typename T>
concept IsNotification = requires() {
  requires true;
};

template <typename T, typename R, typename N>
concept IsCallable = requires {
  { std::is_invocable_r_v<R, T, N> };
  requires IsNotification<N>;
};

template <typename T, typename N>
concept IsNotificationHandler = requires(T t) {
  { std::declval<T&&>() };  // move constructible
  { t.register_to_publisher() } -> std::same_as<bool>;
  { t.template receive(std::declval<std::function<void(N)>>()) };
};

}  // namespace concepts
}  // namespace ipcpp