/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/ipcpp.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>

#include <iostream>

#include "message.h"

std::error_code receive_callback(const Message& msg) {
  const std::int64_t ts = ipcpp::utils::timestamp();
  const std::string_view message(msg.data.data(), msg.data.size());
  if (message == "exit") {
    return {1, std::system_category()};
  }
  std::cout  << "message: " << message << std::endl;
  return {};
}

int main() {
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
  if (!subscriber.subscribe()) {
    std::cerr << "Failed to subscribe"<< std::endl;
    return 1;
  }
  while (true) {
    if (auto message = subscriber.await_message(); message) {
      if (receive_callback(*message)) {
        break;
      }
    }
  }
}