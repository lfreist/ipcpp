/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_subscriber.h>

#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  auto expected_broadcaster = ipcpp::publish_subscribe::BroadcastSubscriber<int>::create("broadcaster");
  if (!expected_broadcaster.has_value()) {
    std::cout << "Error creating client: " << expected_broadcaster.error() << std::endl;
    return 1;
  }
  auto& broadcaster = expected_broadcaster.value();
  spdlog::info("go!");
  for (int i = 0; i < 1000; ++i) {
    broadcaster.on_receive([](ipcpp::publish_subscribe::BroadcastSubscriber<int>::Data<ipcpp::AccessMode::READ>& data) {
      std::cout << data.data() << std::endl;
    });
  }
  return 0;
}