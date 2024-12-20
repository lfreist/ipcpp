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
#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/publish_subscribe/notification.h>
#include <ipcpp/publish_subscribe/publisher.h>
#include <ipcpp/ipcpp.h>

#include <iostream>

#include "message.h"

/*
int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  if (auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<Message,
ipcpp::event::ShmAtomicNotifier<ipcpp::publish_subscribe::Notification>>::create("broadcaster", 10, 2048, 4096);
      expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    while (true) {
      Message message{.timestamp=ipcpp::utils::timestamp(), .message_type=MessageType::REGULAR,
.data=ipcpp::vector<char>()}; std::cout << "Enter message: "; while (true) { char c; std::cin.get(c); if (c == '\n') {
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

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  if (!ipcpp::initialize_dynamic_buffer(4096 * 4096)) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  ipcpp::publish_subscribe::Publisher<Message> publisher("my_id");
  if (std::error_code error = publisher.initialize()) {
    return 1;
  }
  while (true) {
    std::cout << "Enter message: ";
    std::string line;
    std::getline(std::cin, line);
    Message msg;
    msg.data = ipcpp::vector<char>(line.begin(), line.end());
    publisher.publish(std::move(msg));
    if (line == "exit") {
      break;
    }
  }
}
