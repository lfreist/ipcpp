/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
* Author: Leon Freist <freist.leon@gmail.com>
*
* This file is part of ipcpp.
*
* Example usage of DomainSocketNotifier.
*
* Usage:
*  - Start the binary compiled from domain_socket_notifier.cpp to run the notifier
*  - Start the binary compiled from this file to run observers
*  - Press enter to broadcast a notification to all subscribed observers
*/

#include <ipcpp/event/domain_socket_observer.h>
#include <ipcpp/event/notification.h>
#include <ipcpp/utils/utils.h>

#include <chrono>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  using namespace std::chrono_literals;
  auto expected_observer = ipcpp::event::DomainSocketObserver<Notification, ipcpp::event::SubscriptionResponse<int>>::create("/tmp/ipcpp.sock");
  if (!expected_observer.has_value()) {
    std::cerr << "Failed to create DomainSocketObserver: " << expected_observer.error() << std::endl;
    return 1;
  }

  auto& observer = expected_observer.value();

  auto subscription_result = observer.subscribe();
  if (subscription_result.has_value()) {
    std::cout << "Successfully subscribed to Notifier" << std::endl;
  } else {
    std::cerr << "Subscription failed: " << subscription_result.error() << std::endl;
  }

  while (true) {
    auto result = observer.receive([](Notification notification) -> int64_t {
      int64_t timestamp = ipcpp::utils::timestamp();
      return timestamp - notification.timestamp;
    });
    if (result.has_value()) {
      std::cout << "Received notification with latency: " << result.value() << std::endl;
    } else {
      std::cout << "Error receiving message: " << result.error() << std::endl;
      break;
    }
  }
}