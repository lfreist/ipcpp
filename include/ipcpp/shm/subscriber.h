//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/shm/notification.h>
#include <ipcpp/types.h>
#include <ipcpp/subscriber/notification_handler.h>
#include <ipcpp/shm/mapped_memory.h>

#include <string_view>

namespace ipcpp::shm {

class Subscriber {
 public:
  Subscriber(MappedMemory<MappingType::DOUBLE>&& mapped_memory, subscriber::DomainSocketNotificationHandler<Notification>&& notification_handler) : _mapped_memory(std::move(mapped_memory)), _notification_handler(std::move(notification_handler)) {
    if (!_notification_handler.register_to_publisher()) {
      std::cerr << "Registration error" << std::endl;
    }
  }

  void run() {
    while (true) {
      auto exit = _notification_handler.receive([this](Notification notification) { return on_receive_callback(notification); });
      if (exit.has_value()) {
        if (exit.value()) {
          break;
        }
      } else {
        std::cout << "No ret" << std::endl;
      }
    }
  }

  bool on_receive_callback(Notification notification) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t sender_timestamp = _mapped_memory.data<uint64_t>(notification.offset)[0];
    std::string_view msg(_mapped_memory.data<char>() + notification.offset + sizeof(uint64_t), notification.size - sizeof(uint64_t));
    std::cout << "Received " << notification.notification_type << std::endl;
    std::cout << " size   : " << notification.size << " bytes" << std::endl;
    std::cout << " message: " << msg << std::endl;
    std::cout << " latency: " << timestamp - sender_timestamp << " ns" << std::endl;
    return false;
  }

 private:
  MappedMemory<MappingType::DOUBLE> _mapped_memory;
  subscriber::DomainSocketNotificationHandler<Notification> _notification_handler;
};

}