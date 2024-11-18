/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <string>
#include <expected>
#include <chrono>
#include <any>
#include <concepts>

#include <ipcpp/event/notification.h>

namespace ipcpp::event {

template <typename NotificationT, typename SubscriptionRetT = void>
class Observer_I {
 public:
  typedef SubscriptionRetT subscription_return_type;
  typedef NotificationT notification_type;

  typedef NotificationError notification_error_type;  // defined in notification.h

 public:
  Observer_I() = default;
  virtual ~Observer_I() = default;

  /// virtual member function to subscribe receiver to sender
  virtual std::expected<typename subscription_return_type::data_type, typename subscription_return_type::info_type> subscribe() = 0;
  /// virtual member function to unsubscribe receiver from sender
  virtual std::expected<void, typename subscription_return_type::info_type> cancel_subscription() = 0;

  /**
   * @brief Templated wrapper function for virtual receive_helper.
   * @tparam F callback_type
   * @tparam Args callback argument_types
   * @param callback callback
   * @param args callback arguments
   * @return std::expected<return value of callback, notification_error_type>
   */
  template <typename F, typename... Args>
    requires std::is_invocable_v<F, notification_type, Args...>
  auto receive(F&& callback, Args&&... args)
      -> std::expected<decltype(std::forward<F>(callback)(std::declval<notification_type>(), std::forward<Args>(args)...)),
                       notification_error_type> {
    using ReturnType = decltype(std::forward<F>(callback)(std::declval<notification_type>(), std::forward<Args>(args)...));

    std::function<std::any(notification_type)> func =
        [&, callback = std::forward<F>(callback)](notification_type notification) mutable -> std::any {
      if constexpr (std::is_void_v<ReturnType>) {
        std::invoke(callback, notification, std::forward<Args>(args)...);
        return {}; // return empty std::any for void
      } else {
        return std::invoke(callback, notification, std::forward<Args>(args)...);
      }
    };

    auto result = receive_helper(func);

    if (!result) return std::unexpected(result.error());
    if constexpr (std::is_void_v<ReturnType>) {
      return {};
    } else {
      return std::any_cast<ReturnType>(result.value());
    }
  }

 protected:
  /// virtual member function to handle received notifications
  virtual std::expected<std::any, notification_error_type> receive_helper(
      const std::function<std::any(notification_type)>& callback) = 0;
};

}