/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_publisher.h>

#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  auto expected_broadcaster = ipcpp::publish_subscribe::Broadcaster<int>::create("broadcaster", 10, 1024);
  if (expected_broadcaster.has_value()) {
    auto& broadcaster = expected_broadcaster.value();
    for (int i = 0; i < 10000; ++i) {
      broadcaster.publish(i);
      spdlog::info("published {}", i);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

