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
}

}  // namespace ipcpp