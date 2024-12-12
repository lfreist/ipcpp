/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/observer.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/event/notification.h>

#include <spdlog/spdlog.h>

namespace ipcpp::event {

using namespace std::chrono_literals;

template <typename T_Notification>
class ShmAtomicObserver final : public Observer_I<T_Notification> {
 public:
  typedef shm_notification_memory_layout<reference_counted<T_Notification>, shm_atomic_notification_header>
      memory_layout;
  typedef Observer_I<T_Notification> observer_base;

 public:
  explicit ShmAtomicObserver(memory_layout shm_notification_memory) noexcept
      : _memory_layout(shm_notification_memory) {}
  ~ShmAtomicObserver() override = default;

  typename observer_base::subscription_return_type subscribe() override {
    _memory_layout.header->num_subscribers.fetch_add(1, std::memory_order_release);
    _initial_message_number = _memory_layout.header->message_counter.load(std::memory_order_acquire);
    _last_message_number = _initial_message_number;
    observer_base::_subscribed = true;
    return {};
  }

  std::expected<void, SubscriptionError> cancel_subscription() override {
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
    auto msg_counter = _memory_layout.header->message_counter.load(std::memory_order_acquire);
    const auto start_time = std::chrono::high_resolution_clock::now();
    spdlog::debug("_m_receive_helper: _last_message_number: {}, _initial_message_number: {}, message_counter: {}", _last_message_number,
                  _initial_message_number, msg_counter);
    if (_last_message_number == _initial_message_number) {
      // never read a message before
      if (_initial_message_number == -1) {
        // wait for the first notification message
        if (msg_counter > -1) {
          // notifications were sent, we read the first
          spdlog::debug("msg_counter: {}", msg_counter);
          _last_message_number++;
        } else {
          // wait for the first notification to be sent
          spdlog::debug("waiting for message_counter: {}", _last_message_number);
          while (_memory_layout.header->message_counter.load(std::memory_order_acquire) == _last_message_number) {
            std::this_thread::yield();
            if (timeout.count() > 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time) > timeout) {
              return std::unexpected(observer_base::notification_error_type::TIMEOUT);
            }
          }
          _last_message_number++;
        }
      } else {
        // read the initial notification: don't increase the local counter
        spdlog::debug("reading initial message");
        // we decrease the _initial_message_number to avoid reaching this point again
        _initial_message_number--;
      }
    } else {
      // already received notifications: just continue with the next one
      if (msg_counter > _last_message_number) {
        // new notification was sent
        spdlog::debug("msg_counter ({}) > _last_message_number ({})", msg_counter, _last_message_number);
        _last_message_number++;
      } else if (msg_counter < _last_message_number) {
        // should never be reached
        spdlog::error("local message number is larger than actual message number");
      } else {
        // wait for the next notification to be sent
        spdlog::debug("waiting for message number: {}", _last_message_number);
        while (_memory_layout.header->message_counter.load(std::memory_order_acquire) == _last_message_number) {
          std::this_thread::yield();
          if (timeout.count() > 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time) > timeout) {
            return std::unexpected(observer_base::notification_error_type::TIMEOUT);
          }
        }
        _last_message_number++;
      }
    }
    spdlog::debug("using _last_message_number: {}", _last_message_number);
    // TODO: handle already sent notifications after notifier went down
    // if (_notifier_down) {...}
    if (!_memory_layout.header->is_active.load(std::memory_order_acquire)) {
      spdlog::debug("notifier is down!");
      _notifier_down = true;
      if (_last_message_number == _memory_layout.header->last_message_counter) {
        spdlog::warn("ShmAtomicObserver::receive: Notifier is down");
        return std::unexpected(observer_base::notification_error_type::NOTIFIER_DOWN);
      }
    }
    // at this point _last_message_received is the next message we want to read
    // first check that this message is still the one that we expect
    auto* notification = &_memory_layout.message_buffer[_last_message_number];
    while (notification->message_number != _last_message_number) {
      // the read notification has been overwritten, try the next one
      spdlog::debug("notification message number: {}, _last_message_number: {}", notification->message_number, _last_message_number);
      _last_message_number++;
      if (_last_message_number > _memory_layout.header->message_counter.load(std::memory_order_acquire)) {
        // this point can never be reached: we now that _last_message_number once was a valid message number but has
        // been overwritten. By incrementing it, we can never exceed the message counter because this one will always be
        // valid because it cannot be overwritten.
        std::unreachable();
      }
      notification = &_memory_layout.message_buffer[_last_message_number];
    }
    // notification is guaranteed to be valid now. Consume it and pass it to the callback for processing
    spdlog::debug("received notification with message_number {}", notification->message_number);
    auto data = notification->message.consume();
    return callback(*data);
  }

 private:
  memory_layout _memory_layout;
  std::int64_t _last_message_number = -1;
  /// message number at subscription
  std::int64_t _initial_message_number = -1;
  bool _notifier_down = false;
};

}  // namespace ipcpp::event