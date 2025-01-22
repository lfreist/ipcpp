/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <string>
#include <system_error>

namespace carry::event {

enum class error_t {
  success = 0,
  timeout,
  notifier_down,
  observer_down,
  not_subscribed,
  subscription_paused,
  subscription_failed,
  message_invalid,
  notification_failed,
  unknown_error,
};

class error_category final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override { return "carry::event::error_t"; }
  [[nodiscard]] std::string message(int ev) const override {
    switch (static_cast<error_t>(ev)) {
      case error_t::success:
        return "success";
      case error_t::timeout:
        return "timeout";
      case error_t::notifier_down:
        return "notifier_down";
      case error_t::observer_down:
        return "observer_down";
      case error_t::not_subscribed:
        return "not_subscribed";
      case error_t::subscription_paused:
        return "subscription_paused";
      case error_t::subscription_failed:
        return "subscription_failed";
      case error_t::message_invalid:
        return "message_invalid";
      case error_t::notification_failed:
        return "notification_failed";
      case error_t::unknown_error:
        return "unknown_error";
    }
    return "unlisted_error";
  }
};

enum class socket_error_t {
  success = 0,
  create_error,
  bind_error,
  listen_error,
  unknown_error,
};

class socket_error_category final : public std::error_category {
  public:
  [[nodiscard]] const char* name() const noexcept override { return "carry::event::socket_error_t"; }
  [[nodiscard]] std::string message(int ev) const override {
    switch (static_cast<socket_error_t>(ev)) {
      case socket_error_t::success:
        return "success";
      case socket_error_t::create_error:
        return "create_error";
      case socket_error_t::bind_error:
        return "bind_error";
      case socket_error_t::listen_error:
        return "listen_error";
      case socket_error_t::unknown_error:
        return "unknown_error";
    }
    return "unlisted_error";
  }
};

}  // namespace carry::event