//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <expected>
#include <concepts>

#include <ipcpp/types.h>

namespace ipcpp::subscriber {

template <concepts::IsNotification N>
class DomainSocketNotificationHandler {
 public:
  DomainSocketNotificationHandler(DomainSocketNotificationHandler&& other) noexcept {
    std::swap(_socket, other._socket);
    _socket_path = std::move(other._socket_path);
  }

  ~DomainSocketNotificationHandler() {
    if (_socket != -1) {
      close(_socket);
    }
  }

  static std::expected<DomainSocketNotificationHandler, int> create(std::string socket_path) {
    DomainSocketNotificationHandler self(std::move(socket_path));

    if (!self.setup_socket()) {
      return std::unexpected(0);
    }

    return self;
  }

  [[nodiscard]] bool register_to_publisher() const {
    int response;
    recv(_socket, &response, sizeof(response), 0);

    return response == 0;
  }

  template <typename F, typename Ret = bool>
  requires concepts::IsCallable<F, Ret, N>
  std::expected<Ret, int> receive(F&& callback) {
    N notification;

    ssize_t bytes_received = recv(_socket, &notification, sizeof(notification), 0);
    if (bytes_received <= 0) {
      return std::unexpected(0);
    }

    return callback(notification);
  }

 private:
  explicit DomainSocketNotificationHandler(std::string&& socket_path) : _socket_path(std::move(socket_path)) {}

  bool setup_socket() {
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
      return false;
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, _socket_path.c_str(), sizeof(server_addr.sun_path) - 1);

    if (connect(_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      return false;
    }

    return true;
  }

 private:
  std::string _socket_path;
  int _socket = -1;
};

}