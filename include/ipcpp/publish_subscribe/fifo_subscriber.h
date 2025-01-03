/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/observer.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/publish_subscribe/error.h>
#include <ipcpp/publish_subscribe/fifo_message.h>
#include <ipcpp/publish_subscribe/fifo_message_queue.h>
#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/topic.h>
#include <ipcpp/utils/logging.h>

#include <string>

namespace ipcpp::publish_subscribe {

namespace internal {

struct ShmDefault {};

}  // namespace internal

template <typename T_Data, typename T_Observer = internal::ShmDefault>
class Subscriber final {
 public:
  typedef T_Data data_type;
  typedef ps::Message<T_Data> data_access_type;
  typedef T_Observer observer_type;

 public:
  static std::expected<Subscriber, std::error_code> create(const std::string& topic_id,
                                                           const ps::subscriber::Options& options) {
    auto e_topic = get_topic(topic_id);
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_message_queue = ps::shm_message_queue<data_access_type>::read_at(e_topic.value()->shm().addr());
    if (!e_message_queue) {
      return std::unexpected(e_message_queue.error());
    }
    std::string notifications_topic_id(topic_id + "_notifications");
    auto e_observer = T_Observer::create(notifications_topic_id);
    if (!e_observer) {
      return std::unexpected(e_observer.error());
    }

    Subscriber self(std::move(e_topic.value()), options);
    self._message_queue = std::make_unique<ps::shm_message_queue>(std::move(e_message_queue.value()));
    self._observer = std::make_unique<T_Observer>(std::move(e_observer.value()));

    return self;
  }

  std::error_code receive(std::function<std::error_code(const T_Data&)> callback) {
    std::uint64_t msg_id = _observer->receive();
    data_access_type& wrapped_data = _message_queue->operator[](msg_id);
    const auto& o_data = wrapped_data.consume();
    if (!o_data) {
      return std::error_code(1, std::system_category());
    }
    return callback(*o_data.value());
  }

  void subscribe() {
    _observer->subscribe();
  }

  std::error_code cancel_subscription() {
    return _observer->cancel_subscription();
  }

 private:
  Subscriber(Topic&& topic, const ps::subscriber::Options& options) : _topic(std::move(topic)), _options(options) {}

 private:
  Topic _topic = nullptr;
  std::unique_ptr<ps::shm_message_queue<data_access_type>> _message_queue = nullptr;
  ps::subscriber::Options _options;
  std::unique_ptr<observer_type> _observer = nullptr;
};

template <typename T_Data>
class Subscriber<T_Data, internal::ShmDefault> final {
 public:
  typedef T_Data data_type;
  typedef ps::Message<T_Data> data_access_type;

 public:
  static std::expected<Subscriber, std::error_code> create(const std::string& topic_id,
                                                           const ps::subscriber::Options& options = {}) {
    auto e_topic = get_topic(topic_id);
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_message_queue = ps::shm_message_queue<data_access_type>::read_at(e_topic.value()->shm().addr());
    if (!e_message_queue) {
      return std::unexpected(e_message_queue.error());
    }

    Subscriber self(std::move(e_topic.value()), options, std::move(e_message_queue.value()));

    return self;
  }

  std::uint64_t _m_wait_for_data() {
    while (true) {
      if (auto new_msg_id = _message_queue.header()->message_id.next.load(std::memory_order_acquire); new_msg_id != _next_message_id) {
        return new_msg_id;
      }
      // std::this_thread::yield();
    }
  }

  std::error_code receive(std::function<std::error_code(const T_Data&)> callback) {
    std::uint64_t received_message_number = _m_wait_for_data();
    std::uint64_t msg_id = _next_message_id;
    _next_message_id++;
    if (received_message_number == std::numeric_limits<std::uint64_t>::max()) {
      logging::warn("Subscriber<'{}'>::receive(): Publisher down", _topic->id());
      return {1, std::system_category()};
    }
    auto& wrapped_message = _message_queue.operator[](msg_id);
    logging::debug("Subscriber<'{}'>::receive(): received next message: assumed #{}, actual #{}", _topic->id(),
                   msg_id, wrapped_message.message_id());
    if (wrapped_message.message_id() != msg_id) {
      logging::error("Subscriber<'{}'>::receive(): Message invalid: message number mismatch (received: {}, actual: {})",
                     _topic->id(), msg_id, wrapped_message.message_id());
      return {2, std::system_category()};
    }
    auto o_data = wrapped_message.acquire();
    if (!o_data) {
      logging::debug("Subscriber<'{}'>::receive(): data not consumed", _topic->id());
      return {3, std::system_category()};
    }
    return callback(*o_data.value());
  }

  void subscribe() {
    _message_queue.header()->num_subscribers.fetch_add(1, std::memory_order_release);
    _next_message_id = _message_queue.header()->message_id.next.load(std::memory_order_acquire);
  }

  std::error_code cancel_subscription() {
    _message_queue.header()->num_subscribers.fetch_sub(1, std::memory_order_release);
    // TODO: decrease reference counters for messages not yet read
    return {};
  }

 private:
  Subscriber(Topic&& topic, const ps::subscriber::Options& options, ps::shm_message_queue<data_access_type>&& mq) : _topic(std::move(topic)), _options(options), _message_queue(std::move(mq)) {}

 private:
  Topic _topic = nullptr;
  ps::shm_message_queue<data_access_type> _message_queue = nullptr;
  ps::subscriber::Options _options;
  std::uint64_t _next_message_id = 0;
};

}  // namespace ipcpp::publish_subscribe