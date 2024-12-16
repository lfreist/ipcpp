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
  static std::expected<ShmAtomicObserver, factory_error> create(std::string&& id) {
    spdlog::debug("ShmAtomicObserver::create(id='{}')", std::string(id));
    // must be AccessMode::WRITE because of the subscription (increments the subscription counter)
    if (auto result = shm::shared_memory::open<AccessMode::WRITE>("/" + std::string(id) + ".nrb.ipcpp.shm");
        result.has_value()) {
      return ShmAtomicObserver(std::move(result.value()));
    }
    spdlog::error("ShmAtomicObserver::create(id='{}'): Failed to open shared memory segment", std::string(id));
    return std::unexpected(factory_error::SHM_CREATE_FAILED);
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

  std::expected<void, SubscriptionError> pause_subscription() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    observer_base::_subscription_paused = true;
    return {};
  }

  std::expected<void, SubscriptionError> resume_subscription() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    observer_base::_subscription_paused = false;
    return {};
  }

 private:
  explicit ShmAtomicObserver(shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory) noexcept
      : _mapped_memory(std::move(mapped_memory)), _memory_layout(_mapped_memory.addr()) {
    spdlog::debug("ShmAtomicObserver::constructed");
  }
  /**
   * this implementation reads all notifications published since the subscription in sequence
   */
  std::expected<std::any, typename observer_base::notification_error_type> _m_receive_helper(
      const std::function<std::any(typename observer_base::notification_type)>& callback,
      const std::chrono::milliseconds timeout) override {
    if (!observer_base::_subscribed) {
      spdlog::error("ShmAtomicObserver::receive called without being subscribed");
      return std::unexpected(observer_base::notification_error_type::NOT_SUBSCRIBED);
    }
    if (observer_base::_subscription_paused) {
      spdlog::error("ShmAtomicObserver::receive called with subscription being paused");
      return std::unexpected(observer_base::notification_error_type::SUBSCRIPTION_PAUSED);
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
      spdlog::warn("ShmAtomicObserver::receive: Notifier down");
      return std::unexpected(observer_base::notification_error_type::NOTIFIER_DOWN);
    }
    _last_message_number++;
    spdlog::debug("using _last_message_number: {}", _last_message_number);
    auto* notification = &_memory_layout.message_buffer[_last_message_number];
    if (notification->message_number != _last_message_number) {
      spdlog::error("ShmAtomicObserver::receive: Message invalid: message number mismatch");
      return std::unexpected(observer_base::notification_error_type::MESSAGE_INVALID);
    }
    spdlog::debug("received notification with message_number {}", notification->message_number);
    auto data = notification->message.consume();
    return callback(*data);
  }

  bool _m_check_updated() const {
    return _memory_layout.header->message_counter.load(std::memory_order_acquire) != _last_message_number;
  }

 private:
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  memory_layout _memory_layout;
  std::int64_t _last_message_number = -1;
  bool _notifier_down = false;
};

}  // namespace ipcpp::event