/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/config/config.h>
#include <ipcpp/container/fixed_size_queue.h>
#include <ipcpp/container/fixed_size_stack.h>
#include <ipcpp/event/notifier.h>
#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/system.h>

#include <memory>

namespace ipcpp::event {

/**
 * @brief ShmPollingNotifier writes notifications to a shared memory space.
 *
 */
template <typename NotificationT>
class ShmPollingNotifier : public Notifier_I<NotificationT> {
 public:
  struct Config {
    int queue_capacity = 10;
    config::QueueFullPolicy queue_full_policy = config::QueueFullPolicy::DISCARD_OLDEST;
    std::string shm_id;
  };

 public:
  static std::expected<ShmPollingNotifier, int> create(Config&& config) {
    std::size_t required_size = FixedSizeQueue<NotificationT>::required_memory_size(config.queue_capacity);
    required_size = utils::system::round_up_to_pagesize(required_size);
    auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string(config.shm_id), required_size);
    if (expected_mapped_memory.has_value()) {
      auto mapped_memory = std::move(expected_mapped_memory.value());
      auto expected_queue = FixedSizeQueue<NotificationT>::create(
          config.queue_capacity, std::span(mapped_memory.data<uint8_t>(), mapped_memory.size()));
      return ShmPollingNotifier<NotificationT>(std::move(config), std::move(expected_mapped_memory.value()),
                                               std::move(expected_queue.value()));
    }
    return std::unexpected(-1);
  }
  explicit ShmPollingNotifier(Config&& config, shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory,
                              FixedSizeQueue<NotificationT>&& queue)
      : _config(std::move(config)), _mapped_memory(std::move(mapped_memory)), _message_queue(std::move(queue)) {}

  void notify_observers(NotificationT notification) {
    if (_message_queue.is_full()) {
      switch (_config.queue_full_policy) {
        case config::QueueFullPolicy::DISCARD_OLDEST:
          _message_queue.pop_front();
          break;
        case config::QueueFullPolicy::BLOCK:
          while (_message_queue.is_full()) {
            std::this_thread::yield();
          }
          break;
        case config::QueueFullPolicy::ERROR: {
          throw std::runtime_error("Cannot push element to queue: queue is at capacity");
        }
      }
    }
    _message_queue.push(notification);
  }

 private:
  Config _config;
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  FixedSizeQueue<NotificationT> _message_queue;
};

}  // namespace ipcpp::event