/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/event/shm_polling_notifier.h>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  ipcpp::event::ShmPollingNotifier<Notification>::Config config {.queue_capacity=10, .shm_id="ipcpp.shm"};
  auto exected_notifier = ipcpp::event::ShmPollingNotifier<Notification>::create(std::move(config));
}