/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_publisher.h>
#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/utils/io.h>
#include <ipcpp/publish_subscribe/notification.h>

#include <print>

#include "message.h"

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  if (auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<Message, ipcpp::event::ShmAtomicNotifier<ipcpp::publish_subscribe::Notification>>::create("broadcaster", 10, 2048, 4096);
      expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    while (true) {
      Message message{.timestamp=ipcpp::utils::timestamp(), .message_type=MessageType::REGULAR, .data=ipcpp::vector<char>()};
      std::cout << "Enter message: ";
      while (true) {
        char c;
        std::cin.get(c);
        if (c == '\n') {
          break;
        }
        message.data.push_back(c);
      }
      if (message.data == ipcpp::vector<char>({'e', 'x', 'i', 't'})) {
        std::println("sending EXIT message");
        message.message_type = MessageType::EXIT;
        broadcaster.publish(message);
        break;
      }
      std::println("publishing {}", std::string_view(message.data.data(), message.data.size()));
      broadcaster.publish(std::move(message));
    }
  }
}

