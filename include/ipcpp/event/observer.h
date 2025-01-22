/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/event/error.h>

#include <any>
#include <chrono>
#include <expected>
#include <functional>
#include <future>

namespace carry::event {

namespace internal {
template <typename SubscriptionRetT, typename = void>
struct subscription_return_type_helper;

// Specialization for `void`
template <typename SubscriptionRetT>
struct subscription_return_type_helper<SubscriptionRetT, std::enable_if_t<std::is_void_v<SubscriptionRetT>>> {
  using type = std::expected<void, std::error_code>;
};

// Specialization for non-void types with `data_type`
template <typename SubscriptionRetT>
struct subscription_return_type_helper<SubscriptionRetT, std::enable_if_t<!std::is_void_v<SubscriptionRetT>>> {
  using type = std::expected<typename SubscriptionRetT::data_type, std::error_code>;
};
}

template <typename NotificationT, typename SubscriptionRetT = void>
class Observer_I {
 public:
  typedef NotificationT notification_type;
  typedef typename internal::subscription_return_type_helper<SubscriptionRetT>::type subscription_return_type;

 public:
  Observer_I() = default;
  virtual ~Observer_I() = default;

  /// virtual member function to subscribe receiver to sender
  virtual std::error_code subscribe() = 0;
  /// virtual member function to unsubscribe receiver from sender
  virtual std::error_code cancel_subscription() = 0;

  virtual std::expected<void, std::error_code> pause_subscription() = 0;

  virtual std::expected<void, std::error_code> resume_subscription() = 0;

  [[nodiscard]] bool is_subscribed() const noexcept { return _subscribed; }
  [[nodiscard]] bool is_subscription_paused() const noexcept { return _subscription_paused; }

  /**
   * @brief Templated wrapper function for virtual receive_helper.
   * @tparam F callback_type
   * @tparam Args callback argument_types
   * @param timeout timeout
   * @param callback callback
   * @param args callback arguments
   * @return std::expected<return value of callback, notification_error_type>
   */
  template <typename F, typename... Args>
    requires std::is_invocable_v<F, notification_type, Args...>
  auto receive(const std::chrono::milliseconds timeout, F&& callback, Args&&... args)
      -> std::expected<decltype(std::forward<F>(callback)(std::declval<notification_type>(),
                                                          std::forward<Args>(args)...)),
                       std::error_code> {
    using ReturnType =
        decltype(std::forward<F>(callback)(std::declval<notification_type>(), std::forward<Args>(args)...));

    if (!_subscribed) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::not_subscribed), error_category()));
    }
    if (_subscription_paused) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::subscription_paused), error_category()));
    }

    std::function<std::any(notification_type)> func =
        [&, callback = std::forward<F>(callback)](notification_type notification) mutable -> std::any {
      if constexpr (std::is_void_v<ReturnType>) {
        std::invoke(callback, notification, std::forward<Args>(args)...);
        return {};  // return empty std::any for void
      } else {
        return std::invoke(callback, notification, std::forward<Args>(args)...);
      }
    };

    auto result = _m_receive_helper(func, timeout);

    if (!result) return std::unexpected(std::error_code());
    if constexpr (std::is_void_v<ReturnType>) {
      return {};
    } else {
      return std::any_cast<ReturnType>(result.value());
    }
  }

 protected:
  /// virtual member function to handle received notifications
  virtual std::expected<std::any, std::error_code> _m_receive_helper(
      const std::function<std::any(notification_type)>& callback, std::chrono::milliseconds timeout) = 0;

 protected:
  bool _subscribed = false;
  bool _subscription_paused = false;
};

namespace concepts {

template <typename T>
concept is_observer = requires (T observer) {
  { observer.subscribe() } -> std::same_as<std::error_code>;
  { observer.cancel_subscription() } -> std::same_as<std::error_code>;
};

}

}  // namespace carry::event
