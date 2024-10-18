#pragma once

#include <cstring>
#include <expected>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace ipcpp::publisher {

template <template <typename N> typename NotifierT, typename N>
class Publisher {
 public:
  Publisher(Publisher&& other) noexcept : _notifier(std::move(other._notifier)) {}

  ~Publisher() = default;

  template <typename... Args>
  static std::expected<Publisher<NotifierT, N>, typename NotifierT<N>::error> create(Args&&... args) {
    std::expected<NotifierT<N>, typename NotifierT<N>::error> notifier = NotifierT<N>::create(std::forward<Args&&...>(args...));

    if (notifier.has_value()) {
      return Publisher<NotifierT, N>(std::move(notifier.value()));
    }
    return std::unexpected(notifier.error());
  }

  template <typename P>
  void publish(P&& payload) {
    std::cout << "mock published payload " << payload << std::endl;
    N notification;
    notification.size = payload.size();

    _notifier.notify_subscribers(notification);
  }

  void enable_registration() {
    _notifier.enable_registration();
  }

  void disable_registration() {
    _notifier.disable_registration();
  }

 private:
  explicit Publisher(NotifierT<N>&& notifier) : _notifier(std::move(notifier)) {}

  NotifierT<N> _notifier;
};

}  // namespace ipcpp::publisher