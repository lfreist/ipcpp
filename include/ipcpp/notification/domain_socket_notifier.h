/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/notification/notification.h>
#include <ipcpp/notification/notifier.h>
#include <ipcpp/sock/error.h>
#include <ipcpp/utils/synchronized.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <queue>
#include <set>
#include <thread>
#include <chrono>

namespace ipcpp::notification {

using namespace std::chrono_literals;

template <typename NotificationT, typename SubscriptionDataT>
class DomainSocketNotifier : Notifier_I<NotificationT, SubscriptionResponse, SubscriptionDataT> {
 public:
  typedef sock::domain::Error create_error_type;
  typedef Notifier_I<NotificationT, SubscriptionResponse, SubscriptionDataT> notifier_base;

 public:
  /// Move constructor needed for DomainSocketNotificationHandler::create()
  DomainSocketNotifier(DomainSocketNotifier&& other) noexcept : notifier_base(std::move(other)) {
    std::swap(_socket, other._socket);
    std::swap(_max_observers, other._max_observers);
    other._observer_sockets.with_read_lock([&](const std::set<int>& sockets) {
      _observer_sockets.wlock()->insert(sockets.begin(), sockets.end());
    });
    if (other._cancellation_enabled.load()) {
      _cancellation_enabled.store(true);
      _cancellation_thread = std::thread(&DomainSocketNotifier::process_cancellations, this);
      other._cancellation_enabled.store(false);
    }
    if (other._subscription_enabled.load()) {
      _subscription_enabled.store(true);
      _subscription_thread = std::thread(&DomainSocketNotifier::process_subscriptions, this);
      other._subscription_enabled.store(false);
    }
    if (other._subscription_thread.joinable()) other._subscription_thread.join();
    if (other._cancellation_thread.joinable()) other._cancellation_thread.join();
  }

  /// Delete copy constructor
  DomainSocketNotifier(const DomainSocketNotifier&) = delete;

  ~DomainSocketNotifier() {
    if (_socket != -1) {
      shutdown();
      close(_socket);
    }
  }

  static std::expected<DomainSocketNotifier, create_error_type> create(std::string&& id) {
    DomainSocketNotifier self(std::move(id));
    auto result = self.setup_socket();
    if (!result.has_value()) {
      spdlog::error("Failed to create DomainSocketNotifier");
      return std::unexpected(result.error());
    }

    spdlog::debug("DomainSocketNotifier successfully created");
    return self;
  }

  void shutdown() {
    // TODO: send unsubscription messages to all observers
    _subscription_enabled.store(false);
    _cancellation_enabled.store(false);
    ::shutdown(_socket, SHUT_RDWR);
    if (_subscription_thread.joinable()) _subscription_thread.join();
    if (_cancellation_thread.joinable()) _cancellation_thread.join();
  }

  void notify_observers(typename notifier_base::notification_type notification) override {
    _observer_sockets.with_write_lock([&](auto& sockets) {
      auto it = sockets.begin();
      while (it != sockets.end()) {
        int observer_socket = *it;

        if (send(observer_socket, &notification, sizeof(notification), MSG_NOSIGNAL) == -1) {
          if (errno == ECONNRESET || errno == EPIPE) {
            spdlog::warn("Observer {} disconnected. Removing from list.", observer_socket);
            close(observer_socket);
            it = sockets.erase(it);
            continue;
          }
          spdlog::warn("Failed to send notification to observer {}", observer_socket);
        }
        ++it;
      }
    });
  }

  void reject_subscriptions() {
    if (!_subscription_enabled.load()) {
      return;
    }
    _subscription_enabled.store(false);
    if (_subscription_thread.joinable()) _subscription_thread.join();
    if (_observer_sockets.rlock()->empty()) {
      _cancellation_enabled.store(false);
      if (_cancellation_thread.joinable()) _cancellation_thread.join();
    }
  }

  void accept_subscriptions() {
    if (_subscription_enabled.load()) return;
    if (!_cancellation_enabled.load()) {
      _cancellation_enabled.store(true);
      _cancellation_thread = std::thread(&DomainSocketNotifier::process_cancellations, this);
    }
    _subscription_enabled.store(true);
    _subscription_thread = std::thread(&DomainSocketNotifier::process_subscriptions, this);
  }

 private:
  explicit DomainSocketNotifier(std::string&& id) : notifier_base(std::move(id)) {}

  std::expected<void, create_error_type> setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return std::unexpected(create_error_type::CREATION_ERROR);
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, this->_id.c_str(), sizeof(server_addr.sun_path) - 1);
    unlink(this->_id.c_str());

    if (bind(_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      return std::unexpected(create_error_type::BIND_ERROR);
    }

    if (listen(_socket, _max_observers) == -1) {
      return std::unexpected(create_error_type ::LISTEN_ERROR);
    }

    return {};
  }

  void process_subscriptions() {
    while (_subscription_enabled.load()) {
      int client_fd = accept(_socket, nullptr, nullptr);
      if (client_fd >= 0) {
        _observer_sockets.wlock()->insert(client_fd);
        SubscriptionResponse<SubscriptionDataT> response;
        response.info = SubscriptionInfo::OK;
        response.data = 1;
        send(client_fd, &response, sizeof(response), 0);
        spdlog::debug("Registered new client: {}", client_fd);
      }
    }
  }

  void process_cancellations() {
    SubscriptionRequest request;
    while (_cancellation_enabled.load()) {
      _observer_sockets.with_write_lock([&](std::set<int>& sockets) {
        for (auto it = sockets.begin(); it != sockets.end(); ) {
          int client_fd = *it;
          ssize_t bytes = recv(client_fd, &request, sizeof(request), MSG_DONTWAIT);
          if (bytes > 0) {
            switch (request) {
              case SubscriptionRequest::SUBSCRIBE:
                spdlog::info("shit");
                break;
              case SubscriptionRequest::UNSUBSCRIBE:
                spdlog::debug("Received cancellation for {}", client_fd);
                it = sockets.erase(it);
                close(client_fd);
                break;
            }
          } else if (bytes == 0) { // Client closed connection
            spdlog::info("Client {} unexpectedly closed connection", client_fd);
            it = sockets.erase(it);
            close(client_fd);
          } else {
            ++it;
          }
        }
      });
      std::this_thread::sleep_for(10ms);
    }
  }

 private:
  int _socket = -1;
  utils::Synchronized<std::set<int>> _observer_sockets;
  int _max_observers = -1;
  std::atomic_bool _subscription_enabled = false;
  std::atomic_bool _cancellation_enabled = false;
  std::thread _subscription_thread;
  std::thread _cancellation_thread;
};

}  // namespace ipcpp::notification