/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/domain_socket_observer.h>
#include <ipcpp/event/notification.h>
#include <ipcpp/publish_subscribe/subscriber.h>
#include <ipcpp/publish_subscribe/subscription_respons.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/stl/allocator.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/utils/synchronized.h>
#include <ipcpp/utils/utils.h>

#include <iostream>
#include <print>

#include <spdlog/spdlog.h>

namespace ipcpp::publish_subscribe {

using namespace std::chrono_literals;

struct Notification {
  int64_t timestamp;
  std::size_t index;
};

template <typename T_p,
          typename T_Observer =
              event::DomainSocketObserver<Notification, event::SubscriptionResponse<SubscriptionResponseData>>>
class BroadcastSubscriber final : Subscriber_I<T_Observer> {
  typedef Subscriber_I<T_Observer> base_subscriber;

 public:
  typedef T_p value_type;
  typedef reference_counted<T_p> value_access_type;

 public:
  BroadcastSubscriber(BroadcastSubscriber&& other) noexcept
      : Subscriber_I<T_Observer>(std::move(other)),
        _id(std::move(other._id)),
        _mapped_memory(std::move(other._mapped_memory)),
        _message_buffer(std::move(other._message_buffer)),
        _mapped_memory_dyn(std::move(other._mapped_memory_dyn)) {}

  static std::expected<BroadcastSubscriber, int> create(std::string&& id) {
    auto notifier = T_Observer::create(std::string(id));
    if (!notifier.has_value()) {
      return std::unexpected(-1);
    }
    spdlog::debug("BroadcastSubscriber created: subscribing");
    auto expected_result = notifier.value().subscribe();
    if (!expected_result.has_value()) {
      std::cerr << "error: " << expected_result.error() << std::endl;
      exit(1);
    }
    // SubscriptionResponseData result = expected_result.value();
    auto mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::open<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.shm"));
    if (!mapped_memory.has_value()) {
      return std::unexpected(-2);
    }
    auto mapped_memory_dyn = shm::MappedMemory<shm::MappingType::SINGLE>::open<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.dyn.shm"));
    if (!mapped_memory_dyn.has_value()) {
      return std::unexpected(-3);
    }
    BroadcastSubscriber self(std::move(notifier.value()), std::move(mapped_memory.value()),
                             std::move(mapped_memory_dyn.value()));
    self._id = std::move(id);
    return self;
  }

  std::expected<bool, typename T_Observer::notification_error_type> on_receive(std::function<bool(value_access_type&)> callback, const std::chrono::milliseconds timeout = 0ms) {
    auto expected_notification = base_subscriber::_observer.receive(timeout, [](Notification n) { return n; });
    if (!expected_notification.has_value()) {
      spdlog::error("BroadcastSubscriber::on_receive: {}", utils::to_string(expected_notification.error()));
      return std::unexpected(expected_notification.error());
    }
    std::int64_t timestamp_received = utils::timestamp();
    auto& notification = expected_notification.value();
    _m_print_on_receive_info(notification, timestamp_received);
    value_access_type& data = _message_buffer[notification.index];
    return callback(data);
  }

  auto subscribe() -> decltype(base_subscriber::_observer.subscribe()) override {
    return base_subscriber::_observer.subscribe();
  }

  auto cancel_subscription() -> decltype(base_subscriber::_observer.cancel_subscription()) override {
    [[maybe_unused]] auto _ = base_subscriber::_observer.pause_subscription();
    _m_clear_message_buffer();
    return base_subscriber::_observer.cancel_subscription();
  }

  auto pause_subscription() -> decltype(base_subscriber::_observer.pause_subscription()) override {
    return base_subscriber::_observer.pause_subscription();
  }

  auto resume_subscription() -> decltype(base_subscriber::_observer.resume_subscription()) override {
    return base_subscriber::_observer.resume_subscription();
  }

 private:
  BroadcastSubscriber(T_Observer&& observer, shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory,
                      shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory_dyn)
      : Subscriber_I<T_Observer>(std::move(observer)),
        _mapped_memory(std::move(mapped_memory)),
        _message_buffer(reinterpret_cast<std::uintptr_t>(_mapped_memory.addr()), _mapped_memory.size()),
        _mapped_memory_dyn(std::move(mapped_memory_dyn)) {
    pool_allocator<uint8_t>::initialize_factory(reinterpret_cast<void*>(_mapped_memory_dyn.addr()));
  }

  void _m_clear_message_buffer() {
    while (true) {
      if (auto res = on_receive(
              [](value_access_type& data) -> bool {
                data.consume();
                return true;
              },
              10ms);
          !res.has_value()) {
        break;
      }
    }
  }

  static void _m_print_on_receive_info(const Notification& notification, const std::int64_t rec_timestamp) {
    std::println("received data at timestamp: {}", notification.timestamp);
    std::println("                     index: {}", notification.index);
    std::println("                   latency: {}", rec_timestamp - notification.timestamp);
    std::println("                     index: {}", notification.index);
  }

 private:
  std::string _id{};
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  shm::ring_buffer<value_access_type> _message_buffer;
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory_dyn;
};

}  // namespace ipcpp::publish_subscribe