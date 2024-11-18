/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 *
 * Example usage of DomainSocketNotifier.
 *
 * Usage:
 *  - Start the binary compiled from this file to run the notifier
 *  - Start the binary compiled from domain_socket_observer.cpp to run observers
 *  - Press enter to broadcast a notification to all subscribed observers
 */

#include <ipcpp/event/domain_socket_notifier.h>
#include <ipcpp/event/notification.h>
#include <ipcpp/utils/utils.h>

#include <ipcpp/shm/data_view.h>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  // retrieve a DomainSocketNotifier that accepts a maximum of 5 subscribers, broadcasting a Notification
  auto expected_notifier = ipcpp::event::DomainSocketNotifier<Notification, ipcpp::event::SubscriptionResponse<int>>::create("/tmp/ipcpp.sock", 5);
  if (!expected_notifier.has_value()) {
    std::cerr << "Failed to create DomainSocketNotifier: " << expected_notifier.error() << std::endl;
  }

  auto& notifier = expected_notifier.value();

  // accept subscriptions
  notifier.accept_subscriptions();

  while (true) {
    std::cout << "Press 'enter' to send a notification" << std::endl;
    std::string tmp;
    std::getline(std::cin, tmp);

    if (tmp == "exit") {
      break;
    }

    // broadcast the current timestamp
    Notification notification{.timestamp = ipcpp::utils::timestamp()};
    notifier.notify_observers(notification);
  }
}