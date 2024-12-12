/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notification.h>
#include <ipcpp/event/observer.h>
#include <ipcpp/sock/error.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <chrono>

namespace ipcpp::event {

using namespace std::chrono_literals;

template <typename NotificationT, typename SubscriptionDataT>
class DomainSocketObserver final : public Observer_I<NotificationT, SubscriptionDataT> {
 public:
  typedef sock::domain::Error create_error_type;
  typedef Observer_I<NotificationT, SubscriptionDataT> observer_base;

 public:
  /// Move constructor needed for DomainSocketNotificationHandler::create()
  DomainSocketObserver(DomainSocketObserver&& other) noexcept
      : observer_base(std::move(other)), _id(std::move(other._id)) {
    std::swap(_socket, other._socket);
  }

  /// Delete copy constructor
  DomainSocketObserver(const DomainSocketObserver&) = delete;

  ~DomainSocketObserver() override {
    if (_socket != -1) {
      if (observer_base::is_subscribed()) {
        DomainSocketObserver::cancel_subscription().or_else(
            [](auto error) -> std::expected<void, SubscriptionError> {
              return {};
            });
      }
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
    }
  }

  static std::expected<DomainSocketObserver, create_error_type> create(std::string&& id) {
    DomainSocketObserver self(std::move(id));
    return self;
  }

  typename observer_base::subscription_return_type subscribe() override {
    if (observer_base::is_subscription_paused()) {
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
      _socket = -1;
    }

    if (!_m_setup_socket().has_value()) {
      return std::unexpected(SubscriptionError::OBSERVER_SETUP_FAILED);
    }

    SubscriptionDataT response;
    if (recv(_socket, &response, sizeof(response), 0) == -1) {
      return std::unexpected(SubscriptionError::NOTIFIER_UNREACHABLE);
    }

    if (response.info == SubscriptionError::NO_ERROR) {
      observer_base::_subscribed = true;
      return response.data;
    }
    return std::unexpected(response.info);
  }

  std::expected<void, SubscriptionError> cancel_subscription() override {
    if (!observer_base::is_subscribed()) {
      return std::unexpected(SubscriptionError::OBSERVER_NOT_SUBSCRIBED);
    }
    if (observer_base::is_subscription_paused()) {
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
      _socket = -1;
      return {};
    }

    constexpr auto request = ObserverRequest::CANCEL_SUBSCRIPTION;
    if (send(_socket, &request, sizeof(request), 0) == -1) {
      return std::unexpected(SubscriptionError::NOTIFIER_UNREACHABLE);
    }

    auto response = SubscriptionError::UNKNOWN;
    if (recv(_socket, &response, sizeof(response), 0) == -1) {
      return std::unexpected(SubscriptionError::NOTIFIER_UNREACHABLE);
    }

    if (response == SubscriptionError::NO_ERROR) {
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
      _socket = -1;
      return {};
    }

    return std::unexpected(response);
  }

  std::expected<void, SubscriptionError> pause_subscription() override {
    if (!observer_base::is_subscribed()) {
      return std::unexpected(SubscriptionError::OBSERVER_NOT_SUBSCRIBED);
    }
    if (observer_base::is_subscription_paused()) {
      return {};
    }

    constexpr auto request = ObserverRequest::PAUSE_SUBSCRIPTION;
    if (send(_socket, &request, sizeof(request), 0) == -1) {
      return std::unexpected(SubscriptionError::NOTIFIER_UNREACHABLE);
    }

    return {};
  }

  std::expected<void, SubscriptionError> resume_subscription() override {
    if (!observer_base::is_subscribed()) {
      return std::unexpected(SubscriptionError::OBSERVER_NOT_SUBSCRIBED);
    }
    if (observer_base::is_subscription_paused()) {
      return {};
    }

    constexpr auto request = ObserverRequest::RESUME_SUBSCRIPTION;
    if (send(_socket, &request, sizeof(request), 0) == -1) {
      return std::unexpected(SubscriptionError::NOTIFIER_UNREACHABLE);
    }

    return {};
  }

 private:
  explicit DomainSocketObserver(std::string&& id) : _id(std::move(id)) {}

  std::expected<void, create_error_type> _m_setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return std::unexpected(create_error_type::CREATION_ERROR);
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, _id.c_str(), sizeof(server_addr.sun_path) - 1);

    if (connect(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
      return std::unexpected(create_error_type::CONNECTION_ERROR);
    }

    return {};
  }

  std::expected<std::any, typename observer_base::notification_error_type> _m_receive_helper(
      const std::function<std::any(typename observer_base::notification_type)>& callback,
      const std::chrono::milliseconds timeout) override {
    typename observer_base::notification_type notification;

    // Save the current timeout for restoring at the end of this function
    timeval original_timeout{};
    if (timeout.count() > 0) {
      socklen_t len = sizeof(original_timeout);
      getsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &original_timeout, &len);

      // Set the new timeout
      const timeval new_timeout = {timeout.count() / 1000, (timeout.count() % 1000) * 1000};
      if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &new_timeout, sizeof(new_timeout)) < 0) {
        return std::unexpected(NotificationError::TIMEOUT_SETUP_ERROR);
      }
    }

    auto result = recv(_socket, &notification, sizeof(notification), 0);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return std::unexpected(NotificationError::TIMEOUT);
      }
      return std::unexpected(NotificationError::NOTIFICATION_ERROR);
    }
    if (result == 0) {
      return std::unexpected(NotificationError::NOTIFIER_DOWN);
    }

    if (timeout.count() > 0) {
      // Restore the original timeout
      setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &original_timeout, sizeof(original_timeout));
    }

    return callback(notification);
  }

 private:
  /// notifier (server) socket name
  std::string _id;
  /// file-descriptor, this client is reading from
  int _socket = -1;
};

}  // namespace ipcpp::event