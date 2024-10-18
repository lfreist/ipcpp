//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>
#include <ipcpp/notification/notification_handler.h>
#include <ipcpp/types.h>

#include <string_view>

namespace ipcpp::shm {

template <template <typename N> typename NotifierT>
class Subscriber {
 public:
  Subscriber(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notification_handler);

  void run();

  template <typename F, typename R, typename E>
  std::expected<R, E> run(F&& callback);

  bool on_receive_callback(Notification notification);

 private:
  MappedMemory<MappingType::DOUBLE> _mapped_memory;
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
void Subscriber<NotifierT>::run() {
  while (true) {
    auto callback = [this](Notification notification) { return on_receive_callback(notification); };
    auto exit = _notification_handler.template receive<decltype(callback), bool>(std::move(callback));
    if (exit.has_value()) {
      if (exit.value()) {
        break;
      }
    } else {
      std::cout << exit.error() << std::endl;
    }
  }
}

template <template <typename N> typename NotifierT>
template <typename F, typename R, typename E>
std::expected<R, E> Subscriber<NotifierT>::run(F&& callback) {
  return _notification_handler.template receive<F, R, Notification, E>(std::forward<F>(callback));
}

template <template <typename N> typename NotifierT>
bool Subscriber<NotifierT>::on_receive_callback(ipcpp::shm::Notification notification) {
  uint64_t timestamp =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  uint64_t sender_timestamp = _mapped_memory.data<uint64_t>(notification.offset)[0];
  std::string_view msg(_mapped_memory.data<char>() + notification.offset + sizeof(uint64_t),
                       notification.size - sizeof(uint64_t));
  std::cout << "Received " << notification.notification_type << std::endl;
  std::cout << " size   : " << notification.size << " bytes" << std::endl;
  std::cout << " message: " << msg << std::endl;
  std::cout << " latency: " << timestamp - sender_timestamp << " ns" << std::endl;
  return false;
}

}  // namespace ipcpp::shm