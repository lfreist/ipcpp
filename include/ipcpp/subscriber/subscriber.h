#include <ipcpp/notification/notification_handler.h>


#include <atomic>
#include <cstring>
#include <expected>
#include <functional>
#include <iostream>
#include <utility>

namespace ipcpp::subscriber {

template <template <typename N> typename NotificationHandlerT, concepts::IsNotification NotificationT>
class Subscriber {
 public:
  Subscriber(Subscriber&& other) noexcept : _notifier(std::move(other._notifier)) {}

  template <typename... Args>
  static std::expected<Subscriber<NotificationHandlerT, NotificationT>, typename NotificationHandlerT<NotificationT>::error> create(Args&&... args) {
    std::expected<NotificationHandlerT<NotificationT>, typename NotificationHandlerT<NotificationT>::error> notifier =
        NotificationHandlerT<NotificationT>::create(std::forward<Args&&...>(args...));

    if (notifier.has_value()) {
      return Subscriber<NotificationHandlerT, NotificationT>(std::move(notifier.value()));
    }

    return std::unexpected(notifier.error());
  }

  bool register_to_publisher() { return _notifier.register_to_publisher(); }

  template <typename F, typename Ret = bool>
  requires concepts::IsCallable<F, Ret, NotificationT>
  std::expected<Ret, int> receive_one(F&& callback) {
    return _notifier.template receive<F, Ret>(std::forward<F>(callback));
  }

 private:
  explicit Subscriber(NotificationHandlerT<NotificationT>&& notifier) : _notifier(std::move(notifier)) {}

  NotificationHandlerT<NotificationT> _notifier;
};

}  // namespace ipcpp::subscriber