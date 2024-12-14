/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/event/shm_atomic_observer.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/shm/factory.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/utils/utils.h>

#include <spdlog/spdlog.h>

#include <chrono>

using namespace std::chrono_literals;

struct Message {
  std::size_t message_number;
  std::int64_t timestamp;
  std::size_t index;
};

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  auto expected_observer = ipcpp::event::ShmAtomicObserver<Message>::create("shm_notifier");
  if (!expected_observer.has_value()) {
    std::cerr << "ShmNotifier could not be created" << std::endl;
    return 1;
  }
  auto& observer = expected_observer.value();
  observer.subscribe();
  while (true) {
    observer.receive(0ms, [](const Message& message) {
      std::cout << "Message received: " << "\n"
                << "#" << message.message_number << "\n"
                << "latency: " << ipcpp::utils::timestamp() - message.timestamp << std::endl;
    });
  }
}