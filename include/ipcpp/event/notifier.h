/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/error.h>

#include <concepts>
#include <string>

namespace ipcpp::event {

template <typename NotificationT, typename SubscriptionRetT = void>
class Notifier_I {
 public:
  typedef SubscriptionRetT subscription_return_type;
  typedef NotificationT notification_type;

 public:
  Notifier_I() = default;
  virtual ~Notifier_I() = default;

  virtual void notify_observers(notification_type notification) = 0;

  [[nodiscard]] virtual std::size_t num_observers() const = 0;

  virtual void accept_subscriptions() {}
  virtual void reject_subscriptions() {}

 protected:
  std::string _id;
};

}  // namespace ipcpp::event