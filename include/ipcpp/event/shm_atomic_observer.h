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
#include <spdlog/spdlog.h>

#include "shm_atomic_notifier.h"

namespace ipcpp::event {

using namespace std::chrono_literals;

template <typename T_Notification>
class ShmAtomicObserver final : public Observer_I<T_Notification> {
 public:
  typedef shm_notification_memory_layout<reference_counted<T_Notification>, shm_atomic_notification_header>
      memory_layout;
  typedef Observer_I<T_Notification> observer_base;
  typedef T_Notification notification_type;

 public:
  static std::expected<ShmAtomicObserver, std::error_code> create(std::string&& id) {
    spdlog::debug("ShmAtomicObserver::create(id='{}')", std::string(id));
    // must be AccessMode::WRITE because of the subscription (increments the subscription counter)
    if (auto result = shm::shared_memory::open<AccessMode::WRITE>("/" + std::string(id) + ".nrb.ipcpp.shm");
        result.has_value()) {
      return ShmAtomicObserver(std::move(result.value()));
    } else {
      spdlog::error("ShmAtomicObserver::create(id='{}'): Failed to open shared memory segment", std::string(id));
      return std::unexpected(result.error());
    }
  }

  ShmAtomicObserver(ShmAtomicObserver&& other) noexcept
      : observer_base(std::move(other)),
        _mapped_memory(std::move(other._mapped_memory)),
        _memory_layout(std::move(other._memory_layout)),
        _last_message_number(other._last_message_number),
        _notifier_down(other._notifier_down) {
    spdlog::debug("ShmAtomicObserver moved");
  }

  ShmAtomicObserver(const ShmAtomicObserver&) = delete;

  ~ShmAtomicObserver() override = default;

  std::error_code subscribe() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    _last_message_number = _memory_layout.header->message_counter.load(std::memory_order_acquire);
    observer_base::_subscribed = true;
    return {};
  }

  std::error_code cancel_subscription() override {
    _memory_layout.header->num_subscribers.fetch_sub(1, std::memory_order_release);
    observer_base::_subscribed = false;
    return {};
  }

  std::expected<void, std::error_code> pause_subscription() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    observer_base::_subscription_paused = true;
    return {};
  }

  std::expected<void, std::error_code> resume_subscription() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    observer_base::_subscription_paused = false;
    return {};
  }

 private:
  explicit ShmAtomicObserver(shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory) noexcept
      : _mapped_memory(std::move(mapped_memory)), _memory_layout(_mapped_memory.addr()) {}
  /**
   * this implementation reads all notifications published since the subscription in sequence
   */
  std::expected<std::any, std::error_code> _m_receive_helper(
      const std::function<std::any(typename observer_base::notification_type)>& callback,
      const std::chrono::milliseconds timeout) override {
    if (!observer_base::_subscribed) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::not_subscribed), error_category()));
    }
    if (observer_base::_subscription_paused) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::subscription_paused), error_category()));
    }
    std::int64_t received_message_number = 0;
    while (true) {
      received_message_number = _memory_layout.header->message_counter.load(std::memory_order_relaxed);
      if (received_message_number != _last_message_number) {
        break;
      }
      std::this_thread::yield();
    }
    if (received_message_number == -1) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::notifier_down), error_category()));
    }
    _last_message_number++;
    auto* notification = &_memory_layout.message_buffer[_last_message_number];
    if (notification->message_number != _last_message_number) {
      return std::unexpected(std::error_code(static_cast<int>(error_t::message_invalid), error_category()));
    }
    auto data = notification->message.consume();
    return callback(*data);
  }

 private:
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  memory_layout _memory_layout;
  std::int64_t _last_message_number = -1;
  bool _notifier_down = false;
};

}  // namespace ipcpp::event