/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

/*
#include <ipcpp/publish_subscribe/broadcast_publisher.h>
#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/utils/io.h>
*/
#include <ipcpp/publish_subscribe/notification.h>
#include <ipcpp/publish_subscribe/publisher.h>
#include <ipcpp/event/shm_atomic_notifier.h>

#include <print>
#include <iostream>

#include "message.h"

/*
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
*/

int main(int argc, char **argv) {
  ipcpp::publish_subscribe::Publisher<int, ipcpp::event::ShmAtomicNotifier> publisher("my_id");
  if (std::error_code error = publisher.initialize()) {
    return 1;
  }
  std::cout << "press enter to start" << std::endl;
  std::string line;
  std::getline(std::cin, line);
  std::cout << "publishing data..." << std::endl;
  for (int i = 0; i < 10; ++i) {
    publisher.publish(i);
    std::this_thread::sleep_for(100ms);
  }
  for (int i = 0; i < 10; ++i) {
    auto* value = publisher.construct_and_get(i);
    // *value->message = i;
    publisher.publish(value);
    std::this_thread::sleep_for(100ms);
  }
}
