//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/notification/notification_handler.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/utils.h>

#include <string_view>

namespace ipcpp::shm {

template <template <typename N> typename NotifierT>
class Subscriber {
 public:
  typedef MappedMemory<MappingType::DOUBLE> SharedMemory;

 public:
  Subscriber(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notification_handler);

  template <typename F, typename... Args>
    requires std::is_invocable_v<F, Notification, typename Subscriber<NotifierT>::SharedMemory&, Args...>
  auto receive(F&& callback, Args&&... args)
      -> std::expected<decltype(std::forward<F>(callback)(std::declval<Notification>(), std::declval<SharedMemory&>(),
                                                          std::forward<Args>(args)...)),
                       event::NotificationError>;

 private:
  SharedMemory _mapped_memory;
  NotifierT<Notification> _notification_handler;
};

// === implementations =================================================================================================
template <template <typename N> typename NotifierT>
Subscriber<NotifierT>::Subscriber(MappedMemory<MappingType::DOUBLE>&& mapped_memory,
                                  NotifierT<Notification>&& notification_handler)
    : _mapped_memory(std::move(mapped_memory)), _notification_handler(std::move(notification_handler)) {
  if (!_notification_handler.register_to_publisher()) {
    std::cerr << "Registration error" << std::endl;
  }
}

template <template <typename N> typename NotifierT>
template <typename F, typename... Args>
  requires std::is_invocable_v<F, Notification, typename Subscriber<NotifierT>::SharedMemory&, Args...>
auto Subscriber<NotifierT>::receive(F&& callback, Args&&... args)
    -> std::expected<decltype(std::forward<F>(callback)(std::declval<Notification>(), std::declval<SharedMemory&>(),
                                                        std::forward<Args>(args)...)),
                     event::NotificationError> {
  return _notification_handler.template receive<F, Args...>(std::forward<F>(callback), _mapped_memory,
                                                            std::forward<Args>(args)...);
}

}  // namespace ipcpp::shm