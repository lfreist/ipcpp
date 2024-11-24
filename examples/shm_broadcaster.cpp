/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_publisher.h>
#include <ipcpp/utils/io.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

#include "message.h"

int main(int argc, char** argv) {
  auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<Message>::create("broadcaster", 10, 2048, 4096);
  if (expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    while (true) {
      Message message{.timestamp=ipcpp::utils::timestamp(), .data=ipcpp::vector<char>()};
      std::cout << "Enter message: ";
      while (true) {
        char c;
        std::cin.get(c);
        if (c == '\n') {
          break;
        }
        message.data.push_back(c);
      }
      broadcaster.publish(message);
      spdlog::info("published {}", std::string_view(message.data.data(), message.data.size()));
    }
  }
}

