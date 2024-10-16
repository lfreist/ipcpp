//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <concepts>
#include <chrono>
#include <ostream>
#include <expected>

#include <ipcpp/shm/error.h>

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

// === concepts ========================================================================================================

template <typename T>
concept Duration = std::is_same_v<T, std::chrono::duration<typename T::rep, typename T::period>>;

/**
 * SynchronizedData must assure that reading and/or writing data is okay while the object lives
 * @tparam T
 */
template <typename T, template<AccessMode> typename View>
concept SynchronizedData = requires(T t, void* addr, std::chrono::nanoseconds duration) {
  { t.create(addr) } -> std::same_as<std::expected<T, shm::error::AccessError>>;
  { t.mem_size() } -> std::same_as<std::size_t>;
  { t.read_view() } -> std::same_as<View<AccessMode::READ>>;
  { t.write_view() } -> std::same_as<View<AccessMode::WRITE>>;
};

}