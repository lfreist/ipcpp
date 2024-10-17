#include <ipcpp/subscriber/notification_handler.h>


#include <atomic>
#include <cstring>
#include <expected>
#include <functional>
#include <iostream>
#include <utility>

namespace ipcpp::subscriber {

template <template <typename N> typename NotifierT, concepts::IsNotification N>
requires concepts::IsNotificationHandler<NotifierT<N>, N>
class Subscriber {
 public:
  Subscriber(Subscriber&& other) noexcept : _notifier(std::move(other._notifier)) {}

  template <typename... Args>
    requires concepts::HasCreate<NotifierT<N>, int, Args...>
  static std::expected<Subscriber<NotifierT, N>, int> create(Args&&... args) {
    std::expected<NotifierT<N>, int> notifier = NotifierT<N>::create(std::forward<Args&&...>(args...));

    if (notifier.has_value()) {
      return Subscriber<NotifierT, N>(std::move(notifier.value()));
    }

    return std::unexpected(notifier.error());
  }

  bool register_to_publisher() { return _notifier.register_to_publisher(); }

  template <typename F, typename Ret = bool>
  requires concepts::IsCallable<F, Ret, N>
  std::expected<Ret, int> receive_one(F&& callback) {
    return _notifier.template receive<F, Ret>(std::forward<F>(callback));
  }

 private:
  explicit Subscriber(NotifierT<N>&& notifier) : _notifier(std::move(notifier)) {}

  NotifierT<N> _notifier;
};

}  // namespace ipcpp::subscriber