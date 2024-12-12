/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/shm/factory.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

struct Message {
  std::size_t message_number;
  std::int64_t timestamp;
  std::size_t index;
};

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);
  auto expected_notifier = ipcpp::event::ShmAtomicNotifier<Message>::create("shm_notifier", 4096);
  if (!expected_notifier) {
    std::cerr << "Failed to create shm_notifier" << std::endl;
    return 1;
  }
  auto notifier = std::move(expected_notifier.value());
  for (std::size_t i = 0; i < 100; ++i) {
    notifier.notify_observers({i, ipcpp::utils::timestamp(), 0});
    std::this_thread::sleep_for(1000ms);
  }
}