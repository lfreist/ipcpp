/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/domain_socket_notifier.h>
#include <ipcpp/event/domain_socket_observer.h>

namespace ipcpp::event {

enum class NotificationMode {
  POLLING,
  SIGNAL,
  HYBRID
};

template <NotificationMode M>
class Factory {
 public:
  template <typename T, typename D>
  static auto create_notifier() {
    if constexpr (M == NotificationMode::POLLING) {
    } else if constexpr (M == NotificationMode::SIGNAL) {
      return DomainSocketNotifier<T, D>::create();
    } else if constexpr (M == NotificationMode::HYBRID) {

    } else {
      static_assert(false, "Invalid NotificationMode");
    }
  }
 private:
};

}