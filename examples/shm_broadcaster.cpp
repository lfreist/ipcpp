/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/ipcpp.h>
#include <ipcpp/publish_subscribe/real_time/real_time_publisher.h>
#include <ipcpp/utils/logging.h>

#include <chrono>
#include <iostream>
#include <print>

#include "message.h"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
  ipcpp::logging::set_level(ipcpp::logging::level::debug);
  if (ipcpp::initialize_runtime(4096 * 4096)) {
    std::cerr << "Failed to initialize buffer" << std::endl;
    return 1;
  }

  auto e_publisher = ipcpp::ps::RealTimePublisher<Message>::create("pub_sub", {.max_num_observers=3});
  if (!e_publisher) {
    std::cerr << "Error creating publisher" << std::endl;
    return 1;
  }
  auto& publisher = e_publisher.value();
  while (true) {
    std::vector<char> i;
    i.insert(i.begin(), 6);
    std::cout << "Enter message: ";
    std::string line;
    std::getline(std::cin, line);
    ipcpp::vector<char> message{line.begin(), line.end()};
    Message msg(message);
    if (std::error_code error = publisher.publish(std::move(msg)); error) {
      std::cerr << "Error publishing message: " << error.message() << std::endl;
    } else {
      std::println("  > sent: {}", line);
    }
    if (line == "exit") {
      break;
    }
  }
}
