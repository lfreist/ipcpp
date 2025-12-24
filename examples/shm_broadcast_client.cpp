/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/ipcpp.h>
#include <ipcpp/publish_subscribe/real_time/real_time_subscriber.h>

#include <iostream>
#include <print>
#include <chrono>
#include <thread>

#include "message.h"

using namespace std::chrono_literals;

std::error_code receive_callback(const Message& msg) {
  const std::int64_t ts = ipcpp::utils::timestamp();
  const std::string_view message(msg.data.data(), msg.data.size());
  if (message == "exit") {
    return {1, std::system_category()};
  }
  std::println("[{}] [{:>6}ns] message: {}", static_cast<const void*>(&msg), ts - msg.timestamp, message);
  return {};
}

int main(int argc, char** argv) {
  ipcpp::logging::set_level(ipcpp::logging::level::debug);
  if (ipcpp::initialize_runtime()) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  auto e_subscriber = ipcpp::ps::RealTimeSubscriber<Message>::create("pub_sub");
  if (!e_subscriber) {
    std::cerr << "Error creating subscriber: " << e_subscriber.error().value() << std::endl;
    return 1;
  }
  auto& subscriber = e_subscriber.value();
  while (true) {
    if (auto message = subscriber.await_message(); message) {
      if (receive_callback(*message)) {
        break;
      }
      if (argc > 1) {
        std::cout << "Press any key to continue";
        std::cin.get();
        std::cout << "  > released: ";
        receive_callback(*message);
      }
    }
  }
  std::println("bye");
}