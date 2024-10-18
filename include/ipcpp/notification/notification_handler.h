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
#include <ipcpp/sock/error.h>

namespace ipcpp::subscriber {

/**
 * DomainSocketNotificationHandler receives notifications via unix domain sockets. It can be used in Subscribers.
 * @tparam N: Notification type received from publisher. concepts::IsNotification<N> must evaluate to true.
 */
template <concepts::IsNotification N>
class DomainSocketNotificationHandler {
 public:
  typedef sock::domain::Error error;
 public:
  /// Move constructor needed for DomainSocketNotificationHandler::create()
  DomainSocketNotificationHandler(DomainSocketNotificationHandler&& other) noexcept;

  /// closes the socket
  ~DomainSocketNotificationHandler();

  /**
   * Creates an instance of DomainSocketNotificationHandler and returns it. If creating and connecting the socket fails,
   *  a sockets::Error is returned instead
   * @param socket_path
   * @return
   */
  static std::expected<DomainSocketNotificationHandler, error> create(std::string socket_path);

  /**
   * Registers to the publisher
   * @return
   */
  [[nodiscard]] bool register_to_publisher() const;

  template <typename F, typename Ret>
  requires concepts::IsCallable<F, Ret, N>
  std::expected<Ret, int> receive(F&& callback);

 private:
  explicit DomainSocketNotificationHandler(std::string&& socket_path) : _socket_path(std::move(socket_path)) {}

  std::expected<void, error> setup_socket();

 private:
  std::string _socket_path;
  int _socket = -1;
};

// === implementations =================================================================================================
// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
DomainSocketNotificationHandler<N>::DomainSocketNotificationHandler(DomainSocketNotificationHandler<N>&& other) noexcept {
  std::swap(_socket, other._socket);
  _socket_path = std::move(other._socket_path);
}

// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
DomainSocketNotificationHandler<N>::~DomainSocketNotificationHandler() {
  if (_socket != -1) {
    close(_socket);
  }
}

// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
std::expected<DomainSocketNotificationHandler<N>, typename DomainSocketNotificationHandler<N>::error> DomainSocketNotificationHandler<N>::create(std::string socket_path) {
  DomainSocketNotificationHandler self(std::move(socket_path));

  auto result = self.setup_socket();
  if (!result.has_value()) {
    return std::unexpected(result.error());
  }

  return self;
}

// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
bool DomainSocketNotificationHandler<N>::register_to_publisher() const {
  int response;
  recv(_socket, &response, sizeof(response), 0);

  return response == 0;
}

// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
template <typename F, typename Ret>
  requires concepts::IsCallable<F, Ret, N>
std::expected<Ret, int> DomainSocketNotificationHandler<N>::receive(F&& callback) {
  N notification;

  ssize_t bytes_received = recv(_socket, &notification, sizeof(notification), 0);
  if (bytes_received <= 0) {
    return std::unexpected(0);
  }

  return callback(notification);
}

// _____________________________________________________________________________________________________________________
template <concepts::IsNotification N>
std::expected<void, typename DomainSocketNotificationHandler<N>::error> DomainSocketNotificationHandler<N>::setup_socket() {
  _socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (_socket == -1) {
    return std::unexpected(sock::domain::Error::CREATION_ERROR);
  }

  sockaddr_un server_addr{};
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, _socket_path.c_str(), sizeof(server_addr.sun_path) - 1);

  if (connect(_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    return std::unexpected(sock::domain::Error::CONNECTION_ERROR);
  }

  return {};
}

}