/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <string>

namespace ipcpp::notification {

template <typename NotificationT, template <typename T> typename SubscriptionRetT, typename T>
class Notifier_I {
 public:
  typedef SubscriptionRetT<T> subscription_return_type;
  typedef NotificationT notification_type;

 public:
  explicit Notifier_I(std::string&& id) : _id(std::move(id)) {}
  virtual ~Notifier_I() = default;

  virtual void notify_observers(notification_type notification) = 0;

 protected:
  std::string _id;
};

}