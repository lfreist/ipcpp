/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/ipcpp.h>
#include <ipcpp/publish_subscribe/fifo_publisher.h>
#include <ipcpp/utils/logging.h>

#include <chrono>
#include <iostream>

#include "message.h"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
  carry::logging::set_level(carry::logging::level::debug);
  if (carry::initialize_runtime(4096 * 4096)) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  /*
  carry::publish_subscribe::Publisher<Message> publisher("my_topic");
  publisher.set_publish_policy(carry::publish_subscribe::publisher::PublishPolicy::timed, 1s);
  if (std::error_code error = publisher.initialize()) {
    return 1;
  }
   */
  auto e_publisher = carry::publish_subscribe::Publisher<Message>::create("pub_sub");
  if (!e_publisher) {
    std::cerr << "Error creating publisher" << std::endl;
    return 1;
  }
  auto& publisher = e_publisher.value();
  while (true) {
    std::cout << "Enter message: ";
    std::string line;
    std::getline(std::cin, line);
    Message msg;
    msg.data = carry::vector<char>(line.begin(), line.end());
    if (std::error_code error = publisher.publish(std::move(msg)); error) {
      std::cerr << "Error publishing message: " << error.message() << std::endl;
    }
    if (line == "exit") {
      break;
    }
  }
}
