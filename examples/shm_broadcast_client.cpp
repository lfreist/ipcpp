/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/subscriber.h>
#include <ipcpp/event/shm_atomic_observer.h>
#include <ipcpp/ipcpp.h>

#include <iostream>

#include "message.h"

/*
bool on_receive_callback(ipcpp::publish_subscribe::BroadcastSubscriber<Message, ipcpp::event::ShmAtomicObserver<ipcpp::publish_subscribe::Notification>>::value_access_type &message) {
  auto data = message.consume();
  if (data->message_type == MessageType::EXIT) {
    return false;
  }
  std::cout << std::string_view(data->data.data(), data->data.size()) << std::endl;
  return true;
}

int main(int argc, char** argv) {
  std::set<int> a;
  auto expected_client = ipcpp::publish_subscribe::BroadcastSubscriber<Message, ipcpp::event::ShmAtomicObserver<ipcpp::publish_subscribe::Notification>>::create("broadcaster");
  if (!expected_client.has_value()) {
    std::cerr << "Error creating client: " << expected_client.error() << std::endl;
    return 1;
  }
  auto& client = expected_client.value();
  while (true) {
    if (auto result = client.on_receive(on_receive_callback);
        result.has_value()) {
      if (!result.value()) {
        break;
      }
    } else {
      std::cerr << "Error receiving message: " << result.error() << std::endl;
    }
  }
  return 0;
}
 */

std::error_code receive_callback(ipcpp::publish_subscribe::Subscriber<Message, ipcpp::event::ShmAtomicObserver>::data_access_type& data) {
  auto value = data.consume();
  const std::int64_t ts = ipcpp::utils::timestamp();
  const std::string_view message(value->data.data(), value->data.size());
  if (message == "exit") {
    return {0, std::system_category()};
  }
  std::cout  << "latency: " << ts - value->timestamp << " - " << message << std::endl;
  return {};
}

int main() {
  ipcpp::initialize_dynamic_buffer();

  ipcpp::publish_subscribe::Subscriber<Message> subscriber("my_id");
  if (const std::error_code error = subscriber.initialize(); error) {
    std::cerr << "Failed to initialize subscriber" << std::endl;
    return 1;
  }
  if (const auto error = subscriber.subscribe(); error) {
    std::cerr << "Failed to subscribe" << std::endl;
    return 1;
  }
  while (true) {
    if (const auto error = subscriber.receive(receive_callback, 0ms); error) {
      std::cout << "Received error: " << error.category().name() << ": " << error.message() << std::endl;
      break;
    }
  }
};