/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notification.h>
#include <ipcpp/event/observer.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/utils/reference_counted.h>

#include <cstring>
#include <utility>

#include "shm_atomic_notifier.h"

using namespace std::chrono_literals;

namespace ipcpp::event {

class ShmAtomicObserver {
 public:
  typedef std::atomic_uint64_t event_id_type;

 public:
  static std::expected<ShmAtomicObserver, std::error_code> create(const std::string& topic_id) {
    auto e_topic = get_shm_entry(topic_id);
    while (!e_topic) {
      std::this_thread::sleep_for(100ms);
      e_topic = get_shm_entry(topic_id);
    }
    return ShmAtomicObserver(std::move(*e_topic));
  }

  uint64_t receive() {
    uint64_t value = _n_value->load(std::memory_order_acquire);
    while (value == _last_value) {
      std::this_thread::yield();
      value = _n_value->load(std::memory_order_acquire);
    }
    _last_value = value;
    return value;
  }

 private:
  explicit ShmAtomicObserver(Topic&& topic)
      : _topic(std::move(topic)) {
    _last_value = reinterpret_cast<event_id_type*>(_topic->shm().addr())->load(std::memory_order_acquire);
    _n_value = reinterpret_cast<event_id_type*>(_topic->shm().addr());
  }

 private:
  Topic _topic;
  event_id_type* _n_value = nullptr;
  uint64_t _last_value;
};

}  // namespace ipcpp::event