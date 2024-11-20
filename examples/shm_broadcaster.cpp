/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_publisher.h>
#include <ipcpp/utils/io.h>

#include <thread>
#include <chrono>

#include "message.h"

#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<String<32>>::create("broadcaster", 10, 2048);
  if (expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    while (true) {
      String<32> message;
      std::cout << "Enter message: ";
      message.size = ipcpp::io::getline(std::cin, std::span(message.text, message.max_size));
      broadcaster.publish(message);
      spdlog::info("published {}", std::string_view(message.text, message.size));
    }
  }
}

