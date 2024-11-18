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
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace ipcpp::event {

template <typename NotificationT, typename SubscriptionDataT>
class DomainSocketObserver : public Observer_I<NotificationT, SubscriptionDataT> {
 public:
  typedef sock::domain::Error create_error_type;
  typedef Observer_I<NotificationT, SubscriptionDataT> observer_base;

 public:
  /// Move constructor needed for DomainSocketNotificationHandler::create()
  DomainSocketObserver(DomainSocketObserver&& other) noexcept : observer_base(std::move(other)) {
    std::swap(_socket, other._socket);
  }

  /// Delete copy constructor
  DomainSocketObserver(const DomainSocketObserver&) = delete;

  ~DomainSocketObserver() {
    if (_socket != -1) {
      if (_subscribed) {
        cancel_subscription().or_else(
            [](auto error) -> std::expected<void, typename observer_base::subscription_return_type::info_type> {
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

  std::expected<typename DomainSocketObserver::subscription_return_type::data_type,
                typename DomainSocketObserver::subscription_return_type::info_type>
  subscribe() override {
    spdlog::debug("subscribing...");
    auto result = setup_socket();
    if (!result.has_value()) {
      return std::unexpected(SubscriptionInfo::OBSERVER_SOCKET_SETUP_FAILED);
    }

    SubscriptionDataT response;
    if (recv(_socket, &response, sizeof(response), 0) == -1) {
      spdlog::warn("no answer received");
      return std::unexpected(SubscriptionInfo::NOTIFIER_UNREACHABLE);
    }

    if (response.info == DomainSocketObserver::subscription_return_type::info_type::OK) {
      spdlog::debug("successfully subscribed");
      _subscribed = true;
      return response.data;
    }
    spdlog::warn("subscription rejected");
    return std::unexpected(response.info);
  }

  std::expected<void, typename observer_base::subscription_return_type::info_type> cancel_subscription() override {
    spdlog::debug("cancelling subscription...");
    if (!_subscribed) {
      spdlog::warn("not subscribed");
      return std::unexpected(SubscriptionInfo::OBSERVER_NOT_SUBSCRIBED);
    }
    ObserverRequest request = ObserverRequest::CANCEL_SUBSCRIPTION;
    if (send(_socket, &request, sizeof(request), 0) == -1) {
      spdlog::warn("sending unsubscription failed");
      return std::unexpected(SubscriptionInfo::NOTIFIER_UNREACHABLE);
    }

    SubscriptionInfo response = SubscriptionInfo::UNKNOWN;
    if (recv(_socket, &response, sizeof(response), 0) == -1) {
      spdlog::warn("no answer received");
      return std::unexpected(SubscriptionInfo::NOTIFIER_UNREACHABLE);
    }

    if (response == SubscriptionInfo::OK) {
      spdlog::info("successfully unsubscribed");
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
      _socket = -1;
      return {};
    }

    return std::unexpected(response);
  }

 private:
  explicit DomainSocketObserver(std::string&& id) : _id(std::move(id)) {}

  std::expected<void, create_error_type> setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return std::unexpected(create_error_type::CREATION_ERROR);
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, this->_id.c_str(), sizeof(server_addr.sun_path) - 1);

    if (connect(_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      return std::unexpected(create_error_type::CONNECTION_ERROR);
    }

    return {};
  }

  std::expected<std::any, typename observer_base::notification_error_type> receive_helper(
      const std::function<std::any(typename observer_base::notification_type)>& callback) override {
    typename observer_base::notification_type notification;

    ssize_t bytes_received = recv(_socket, &notification, sizeof(notification), 0);
    if (bytes_received == -1) {
      return std::unexpected(NotificationError::NOTIFICATION_ERROR);
    } else if (bytes_received == 0) {
      return std::unexpected(NotificationError::NOTIFIER_DOWN);
    }

    return callback(notification);
  };

 private:
  /// socket name
  std::string _id;
  int _socket = -1;
  bool _subscribed = false;
};

}  // namespace ipcpp::notification