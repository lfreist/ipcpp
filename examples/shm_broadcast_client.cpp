/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_subscriber.h>

#include <spdlog/spdlog.h>

#include "message.h"

int main(int argc, char** argv) {
  auto expected_broadcaster = ipcpp::publish_subscribe::BroadcastSubscriber<String<32>>::create("broadcaster");
  if (!expected_broadcaster.has_value()) {
    std::cout << "Error creating client: " << expected_broadcaster.error() << std::endl;
    return 1;
  }
  auto& broadcaster = expected_broadcaster.value();
  spdlog::info("go!");
  for (int i = 0; i < 1000; ++i) {
    broadcaster.on_receive([](ipcpp::publish_subscribe::BroadcastSubscriber<String<32>>::Data<ipcpp::AccessMode::READ>& data) {
      std::cout << std::string_view(data.data().text, data.data().size) << std::endl;
    });
  }
  return 0;
}