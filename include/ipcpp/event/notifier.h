/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <concepts>
#include <string>

namespace ipcpp::event {

template <typename NotificationT>
class Notifier_I {
 public:
  typedef NotificationT notification_type;

 public:
  Notifier_I() = default;
  virtual ~Notifier_I() = default;

  virtual void notify_observers(notification_type notification) = 0;

  [[nodiscard]] virtual std::size_t num_observers() const = 0;

  virtual void accept_subscriptions() {}
  virtual void reject_subscriptions() {}

  std::string_view identifier() { return _id; }

 protected:
  std::string _id;
};

namespace concepts {

template <typename T>
concept is_notifier = requires (T notifier, typename T::notification_type notification) {
 { notifier.notify_observers(notification) } -> std::same_as<void>;
 { notifier.num_observers() } -> std::convertible_to<std::size_t>;
 { notifier.identifier() } -> std::convertible_to<std::string_view>;
};

}

}  // namespace ipcpp::event