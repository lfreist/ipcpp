/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/notification/domain_socket_notifier.h>

#include <ipcpp/utils/utils.h>
#include <chrono>

#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  spdlog::debug("starting DomainSocketNotifier...");
  auto expected_notifier = ipcpp::notification::DomainSocketNotifier<Notification, int>::create("/tmp/ipcpp.sock");
  if (!expected_notifier.has_value()) {
    std::cerr << "Failed to create DomainSocketNotifier: " << expected_notifier.error() << std::endl;
  }

  auto notifier = std::move(expected_notifier.value());
  notifier.accept_subscriptions();

  for (int i = 0; i < 4; ++i) {
    Notification notification {.timestamp = ipcpp::utils::timestamp() };
    notifier.notify_observers(notification);
    spdlog::debug("sending notification {} at {}", i, notification.timestamp);
    std::this_thread::sleep_for(1s);
  }
}