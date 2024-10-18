//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string>
#include <expected>
#include <mutex>
#include <thread>
#include <iostream>

#include <vector>

#include <ipcpp/types.h>
#include <ipcpp/sock/error.h>

namespace ipcpp::publisher {


template <typename N>
class DomainSocketNotifier {
 public:
  typedef sock::domain::Error error;

 public:
  DomainSocketNotifier(DomainSocketNotifier&& other) noexcept {
    // mutex is not moved
    std::swap(_socket, other._socket);
    std::swap(_max_subscribers, other._max_subscribers);
    _subscribers = std::move(other._subscribers);
    _socket_path = std::move(other._socket_path);
  }

  ~DomainSocketNotifier() {
    disable_registration();
    notify_exit();
    if (_socket != -1) {
      close(_socket);
      unlink(_socket_path.c_str());
    }
  }

  static std::expected<DomainSocketNotifier, error> create(std::string&& socket_path) {
    DomainSocketNotifier self(std::move(socket_path));

    auto result = self.setup_socket();
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }

    return self;
  }

  void notify_subscribers(N notification) {
    std::lock_guard<std::mutex> lock(_subscribers_mutex);

    for (auto& subscriber : _subscribers) {
      send_notification(subscriber, notification);
    }
  }

  void enable_registration() {
    if (_registration_enabled.load()) {
      // already running...
      return;
    }
    _registration_enabled.store(true),
        _registration_thread = std::thread([this]() {
          while (true) {
            if (!_registration_enabled.load()) {
              break;
            }
            sockaddr_un client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            fd_set read_fds;

            FD_ZERO(&read_fds);
            FD_SET(_socket, &read_fds);
            struct timeval timeout {};
            timeout.tv_sec = 0;
            timeout.tv_usec = 500;

            int result = select(_socket + 1, &read_fds, nullptr, nullptr, &timeout);

            if (result < 0) {
              std::cerr << "Error in select" << std::endl;
              continue;
            } else if (result == 0) {
              // timeout reached
              continue;
            }

            if (FD_ISSET(_socket, &read_fds)) {
              int client_sock = accept(_socket, (struct sockaddr*)&client_addr, &client_addr_len);

              if (client_sock == -1) {
                std::cerr << "Failed to accept registration request" << std::endl;
                continue;
              }
              handle_registration(client_sock);
            }
          }
        });
  }

  void disable_registration() {
    if (_registration_enabled.load()) {
      _registration_enabled.store(false);
      if (_registration_thread.joinable()) {
        _registration_thread.join();
      }
    }
  }

  void notify_exit() {
    N notification;
    notification.notification_type = notification::NotificationType::EXIT;
    notify_subscribers(notification);
  }

 private:
  explicit DomainSocketNotifier(std::string&& socket_path) : _socket_path(std::move(socket_path)) {}

  std::expected<void, error> setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return std::unexpected(error::CREATION_ERROR);
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, _socket_path.c_str(), sizeof(server_addr.sun_path) - 1);
    unlink(_socket_path.c_str());

    if (bind(_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      return std::unexpected(error::BIND_ERROR);
    }

    if (listen(_socket, _max_subscribers) == -1) {
      return std::unexpected(error::LISTEN_ERROR);
    }

    return {};
  }

  void handle_registration(int client_sock) {
    std::lock_guard<std::mutex> lock(_subscribers_mutex);

    if (_subscribers.size() >= _max_subscribers) {
      int error_code = -1;
      send(client_sock, &error_code, sizeof(error_code), 0);
      close(client_sock);
    } else {
      _subscribers.push_back(client_sock);
      int success_code = 0;
      send(client_sock, &success_code, sizeof(success_code), 0);
    }
  }

  void send_notification(int client_sock, N notification) {
    send(client_sock, &notification, sizeof(notification), 0);
  }

  std::string _socket_path;
  int _socket = -1;
  std::vector<int> _subscribers;
  std::mutex _subscribers_mutex;
  int _max_subscribers = -1;
  std::atomic_bool _registration_enabled = false;
  std::thread _registration_thread;
};

}