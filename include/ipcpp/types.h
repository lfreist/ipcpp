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

std::ostream& operator<<(std::ostream& os, AccessMode am);

namespace event {

enum class NotificationType {
  UNINITIALIZED,
  REGISTRATION_SUCCESS,
  REGISTRATION_FAILED,
  PUBLISHER_DOWN,
  REGULAR,
  EXIT,
};

std::ostream& operator<<(std::ostream& os, NotificationType nt);


enum class _NotificationError {
  NO_DATA,
  PROVIDER_DOWN,
};

std::ostream& operator<<(std::ostream& os, _NotificationError error);
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

}  // namespace concepts
}  // namespace ipcpp