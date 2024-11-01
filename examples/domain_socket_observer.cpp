/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/notification/domain_socket_observer.h>
#include <ipcpp/utils/utils.h>
#include <spdlog/spdlog.h>

#include <chrono>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  using namespace std::chrono_literals;
  spdlog::set_level(spdlog::level::debug);
  auto expected_observer = ipcpp::notification::DomainSocketObserver<Notification, int>::create("/tmp/ipcpp.sock");
  if (!expected_observer.has_value()) {
    std::cerr << "Failed to create DomainSocketObserver: " << expected_observer.error() << std::endl;
    return 1;
  }

  auto observer = std::move(expected_observer.value());
  {
    auto result = observer.subscribe();
    if (result.has_value()) {
      std::cout << result.value() << std::endl;
    } else {
      std::cerr << "Subscription failed: " << result.error() << std::endl;
    }
  }

  auto result = observer.receive([](Notification notification) {
    int64_t timestamp = ipcpp::utils::timestamp();
    std::cout << timestamp - notification.timestamp << std::endl;
  });

  result = observer.receive([](Notification notification) {
    int64_t timestamp = ipcpp::utils::timestamp();
    std::cout << timestamp - notification.timestamp << std::endl;
  });

  observer.cancel_subscription();

  std::this_thread::sleep_for(1s);

  observer.subscribe();

  result = observer.receive([](Notification notification) {
    int64_t timestamp = ipcpp::utils::timestamp();
    std::cout << timestamp - notification.timestamp << std::endl;
  });

  std::this_thread::sleep_for(1s);
}