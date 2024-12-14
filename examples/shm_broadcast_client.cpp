/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/broadcast_subscriber.h>
#include <ipcpp/event/shm_atomic_observer.h>

#include <set>

#include "message.h"

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