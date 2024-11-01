/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/notification/observer.h>
#include <ipcpp/sock/error.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <ipcpp/notification/notification.h>

#include <spdlog/spdlog.h>

namespace ipcpp::notification {

template <typename NotificationT, typename SubscriptionDataT>
class DomainSocketObserver : public Observer_I<NotificationT, SubscriptionResponse, SubscriptionDataT> {
 public:
  typedef sock::domain::Error create_error_type;
  typedef Observer_I<NotificationT, SubscriptionResponse, SubscriptionDataT> observer_base;

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
        cancel_subscription();
      }
      ::shutdown(_socket, SHUT_RDWR);
      close(_socket);
    }
  }

  static std::expected<DomainSocketObserver, create_error_type> create(std::string&& id) {
    DomainSocketObserver self(std::move(id));
    return self;
  }

  std::expected<typename DomainSocketObserver::subscription_return_type::data_type, typename DomainSocketObserver::subscription_return_type::info_type> subscribe() override {
    spdlog::debug("subscribing...");
    setup_socket();

    SubscriptionResponse<SubscriptionDataT> response;
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
    SubscriptionRequest request = SubscriptionRequest::UNSUBSCRIBE;
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
  explicit DomainSocketObserver(std::string&& id) : observer_base(std::move(id)) {}

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
      return std::unexpected(NotificationError::SENDER_DOWN);
    } else if (bytes_received == 0) {
      return std::unexpected(NotificationError::INVALID_NOTIFICATION);
    }

    return callback(notification);
  };

 private:
  int _socket = -1;
  bool _subscribed = false;
};

}