/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/event/shm_polling_observer.h>

struct Notification {
  int64_t timestamp;
};

int main(int argc, char** argv) {
  auto exected_notifier = ipcpp::event::ShmPollingObserver<Notification>::create(std::move(config));
}