/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notifier.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/shm/mapped_memory.h>

#include <expected>
#include <utility>

namespace ipcpp::event {

class ShmAtomicNotifier final {
  // static_assert(is_notifier<ShmAtomicNotifier<T_p>>);
 public:
  typedef std::atomic_uint64_t event_id_type;

 private:

 public:
  ShmAtomicNotifier(ShmAtomicNotifier&& other) = default;

  ShmAtomicNotifier(const ShmAtomicNotifier& other) = delete;

  static std::expected<ShmAtomicNotifier, std::error_code> create(const std::string& topic_id) {
    auto e_topic = get_shm(topic_id, sizeof(event_id_type));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }

    return ShmAtomicNotifier(std::move(*e_topic));
  }

  void notify_observers(const uint64_t add_val = 1) {
    _n_value->fetch_add(add_val, std::memory_order_relaxed);
  }

 private:
  explicit ShmAtomicNotifier(Topic&& topic) : _topic(std::move(topic)) {
    _n_value = reinterpret_cast<event_id_type*>(_topic->shm().addr());
  }

 private:
  Topic _topic;
  event_id_type* _n_value = nullptr;
};

}  // namespace ipcpp::event