/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/ipcpp.h>
#include <ipcpp/publish_subscribe/fifo_subscriber.h>

#include <iostream>

#include "message.h"

std::error_code receive_callback(const Message& msg) {
  const std::int64_t ts = carry::utils::timestamp();
  const std::string_view message(msg.data.data(), msg.data.size());
  if (message == "exit") {
    return {1, std::system_category()};
  }
  std::cout  << "message: " << message << std::endl;
  return {};
}

int main() {
  carry::logging::set_level(carry::logging::level::debug);
  if (carry::initialize_runtime()) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  auto e_subscriber = carry::publish_subscribe::Subscriber<Message>::create("pub_sub");
  if (!e_subscriber) {
    std::cerr << "Error creating subscriber: " << e_subscriber.error().value() << std::endl;
    return 1;
  }
  auto& subscriber = e_subscriber.value();
  if (const auto error = subscriber.subscribe(); error) {
    std::cerr << "Failed to subscribe: " << error.value() << std::endl;
    return 1;
  }
  while (true) {
    if (const auto error = subscriber.receive(receive_callback); error) {
      std::cout << "received exit message" << std::endl;
      break;
    }
  }
}