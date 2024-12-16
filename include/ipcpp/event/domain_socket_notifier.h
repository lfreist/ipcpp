/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notification.h>
#include <ipcpp/event/notifier.h>
#include <ipcpp/utils/synchronized.h>
#include <ipcpp/event/error.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <limits>
#include <queue>
#include <set>
#include <thread>
#include <expected>

namespace ipcpp::event {

using namespace std::chrono_literals;

/**
 * @brief DomainSocketNotifier implement Notifier_I using unix domain sockets (AF_UNIX, SOCK_STREAM). Sent notifications
 * are queued up and observers receive notifications in a FIFO-Queue manner. DomainSocketNotifier::create constructs an
 * instance of DomainSocketNotifier and returns it wrapped by std::expected.
 *
 * @attention If you only want observers to receive the latest data consider using ShmPollingNotifier with
 * ShmPollingObserver.
 *
 * @implements Notifier_I
 *
 * @tparam NotificationT
 * @tparam SubscriptionDataT
 */
template <typename NotificationT>
class DomainSocketNotifier final : Notifier_I<NotificationT> {
 public:
  typedef Notifier_I<NotificationT> notifier_base;

 public:
  /// Move constructor needed for DomainSocketNotificationHandler::create()
  DomainSocketNotifier(DomainSocketNotifier&& other) noexcept
      : notifier_base(std::move(other)),
        _max_observers(other._max_observers) {
    if (other._cancellation_enabled.load()) {
      _cancellation_enabled.store(true);
      _request_processing_thread = std::thread(&DomainSocketNotifier::process_observer_requests, this);
      other._cancellation_enabled.store(false);
    }
    if (other._subscription_enabled.load()) {
      _subscription_enabled.store(true);
      _subscription_processing_thread = std::thread(&DomainSocketNotifier::process_subscriptions, this);
      other._subscription_enabled.store(false);
    }
    if (other._subscription_processing_thread.joinable()) other._subscription_processing_thread.join();
    if (other._request_processing_thread.joinable()) other._request_processing_thread.join();

    // once the processing threads are done working, we move the _observer_sockets
    std::swap(_socket, other._socket);
    other._observer_sockets.with_read_lock(
        [&](const std::set<int>& sockets) { _observer_sockets.wlock()->insert(sockets.begin(), sockets.end()); });
  }

  /// Delete copy constructor
  DomainSocketNotifier(const DomainSocketNotifier&) = delete;

  ~DomainSocketNotifier() override {
    if (_socket != -1) {
      shutdown();
      close(_socket);
    }
  }

  /**
   * @brief Create an instance of DomainSocketNotifier using id as socket path and max_num_observers as maximum number
   *  of concurrent observers.
   *
   * @param id socket path
   * @param max_num_observers maximum number of observers
   * @return instance wrapped by std::expected
   */
  static std::expected<DomainSocketNotifier, std::error_code> create(
      std::string&& id, std::uint16_t max_num_observers = std::numeric_limits<uint16_t>::max()) {
    DomainSocketNotifier self(std::move(id), max_num_observers);
    if (auto result = self.setup_socket(); !result.has_value()) {
      return std::unexpected(result.error());
    }

    return self;
  }

  /**
   * @brief joins processing threads and shuts down the socket
   */
  void shutdown() {
    // TODO: send unsubscription messages to all observers
    _subscription_enabled.store(false);
    _cancellation_enabled.store(false);
    ::shutdown(_socket, SHUT_RDWR);
    if (_subscription_processing_thread.joinable()) _subscription_processing_thread.join();
    if (_request_processing_thread.joinable()) _request_processing_thread.join();
  }

  /**
   * @brief Broadcasts notification to all subscribed observers
   * @param notification
   */
  void notify_observers(typename notifier_base::notification_type notification) override {
    _observer_sockets.with_write_lock([&notification](auto& sockets) {
      auto it = sockets.begin();
      while (it != sockets.end()) {
        int observer_socket = *it;

        if (send(observer_socket, &notification, sizeof(notification), MSG_NOSIGNAL) == -1) {
          if (errno == ECONNRESET || errno == EPIPE) {
            close(observer_socket);
            it = sockets.erase(it);
            continue;
          }
        }
        ++it;
      }
    });
  }

  /**
   * @brief ignore all incoming subscription requests
   */
  void reject_subscriptions() {
    if (!_subscription_enabled.load()) {
      return;
    }
    _subscription_enabled.store(false);
    if (_subscription_processing_thread.joinable()) _subscription_processing_thread.join();
    if (_observer_sockets.rlock()->empty()) {
      _cancellation_enabled.store(false);
      if (_request_processing_thread.joinable()) _request_processing_thread.join();
    }
  }

  /**
   * @brief process incoming subscription requests
   */
  void accept_subscriptions() {
    if (_subscription_enabled.load()) return;
    if (!_cancellation_enabled.load()) {
      _cancellation_enabled.store(true);
      _request_processing_thread = std::thread(&DomainSocketNotifier::process_observer_requests, this);
    }
    _subscription_enabled.store(true);
    _subscription_processing_thread = std::thread(&DomainSocketNotifier::process_subscriptions, this);
  }

  [[nodiscard]] std::size_t num_observers() const override { return _observer_sockets.rlock()->size(); }

 private:
  /// Constructor only used by DomainSocketNotifier::create()
  explicit DomainSocketNotifier(std::string&& id, const uint16_t max_num_observers)
      : _id("/tmp/" + std::move(id) + ".ipcpp.sock"), _max_observers(max_num_observers) {}

  /**
   * @brief Set up the subscription request socket
   * @return
   */
  std::expected<void, std::error_code> setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return std::unexpected(std::error_code(static_cast<int>(socket_error_t::create_error), socket_error_category()));
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, this->_id.c_str(), sizeof(server_addr.sun_path) - 1);
    unlink(this->_id.c_str());

    if (bind(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
      return std::unexpected(std::error_code(static_cast<int>(socket_error_t::bind_error), socket_error_category()));
    }

    if (listen(_socket, _max_observers) == -1) {
      return std::unexpected(std::error_code(static_cast<int>(socket_error_t::listen_error), socket_error_category()));
    }

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return {};
  }

  /**
   * @brief Process subscription requests. Should run on a separate thread (accept_subscriptions() runs it on
   *  _subscription_processing_thread).
   */
  void process_subscriptions() {
    while (_subscription_enabled.load()) {
      if (int client_fd = accept(_socket, nullptr, nullptr); client_fd >= 0) {
        _observer_sockets.wlock()->insert(client_fd);
        bool response = true;
        send(client_fd, &response, sizeof(response), 0);
      }
    }
  }

  /**
   * @brief Process subscribed observer requests (e.g. cancellation of subscriptions). Should run on a separate thread
   *  (accept_subscriptions() runs it on _request_processing_thread).
   */
  void process_observer_requests() {
    ObserverRequest request;
    while (_cancellation_enabled.load()) {
      _observer_sockets.with_write_lock([&](std::set<int>& sockets) {
        for (auto it = sockets.begin(); it != sockets.end();) {
          const int client_fd = *it;
          if (const ssize_t bytes = recv(client_fd, &request, sizeof(request), MSG_DONTWAIT); bytes > 0) {
            switch (request) {
              case ObserverRequest::SUBSCRIBE:
                break;
              case ObserverRequest::CANCEL_SUBSCRIPTION:
                it = sockets.erase(it);
                close(client_fd);
                break;
              case ObserverRequest::PAUSE_SUBSCRIPTION:
                _paused_observer_sockets.wlock()->insert(client_fd);
                it = sockets.erase(it);
                break;
              case ObserverRequest::RESUME_SUBSCRIPTION:
                break;
            }
          } else if (bytes == 0) {
            it = sockets.erase(it);
            close(client_fd);
          } else {
            ++it;
          }
        }
      });
      _paused_observer_sockets.with_write_lock([&](std::set<int>& sockets) {
        for (auto it = sockets.begin(); it != sockets.end();) {
          const int client_fd = *it;
          if (const ssize_t bytes = recv(client_fd, &request, sizeof(request), MSG_DONTWAIT); bytes > 0) {
            switch (request) {
              case ObserverRequest::SUBSCRIBE:
                break;
              case ObserverRequest::CANCEL_SUBSCRIPTION:
                it = sockets.erase(it);
                close(client_fd);
                break;
              case ObserverRequest::PAUSE_SUBSCRIPTION:
                break;
              case ObserverRequest::RESUME_SUBSCRIPTION:
                _observer_sockets.wlock()->insert(client_fd);
                it = sockets.erase(it);
            }
          } else if (bytes == 0) {
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
  /// socket name
  std::string _id;
  /// maximum number of concurrently subscribed observers
  const uint16_t _max_observers;
  /// main socket receiving subscription requests
  int _socket = -1;
  /// file descriptors of subscribed observers
  synchronized<std::set<int>> _observer_sockets;
  synchronized<std::set<int>> _paused_observer_sockets;

  std::atomic_bool _subscription_enabled = false;
  std::atomic_bool _cancellation_enabled = false;
  std::thread _subscription_processing_thread;
  std::thread _request_processing_thread;
};

}  // namespace ipcpp::event