/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/publish_subscribe/message_buffer.h>
#include <ipcpp/topic.h>

#include <optional>

namespace ipcpp::ps {

template <typename T_p>
class RealTimeSubscriber {
 public:
  typedef rt::Message<T_p> message_type;
  typedef typename message_type::Access access_type;

 public:
  static std::expected<RealTimeSubscriber, std::error_code> create(const std::string& topic_id,
                                                                   const subscriber::Options& options = {}) {
    auto e_topic = get_topic(topic_id);
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer = message_buffer<message_type>::read_at(e_topic.value()->shm().addr());
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    RealTimeSubscriber self(std::move(e_topic.value()), options, std::move(e_buffer.value()));

    return self;
  }

 public:
  void subscribe() {
    // TODO: does memory order matter here?
    _message_buffer.header()->num_subscribers.fetch_add(1, std::memory_order_relaxed);
    _initial_message_info = _message_buffer.header()->message_id.published.load(std::memory_order_relaxed);
  }

  void unsubscribe() { _message_buffer.header()->num_subscribers.fetch_sub(1, std::memory_order_relaxed); }

  std::optional<access_type> fetch_message() {
    // TODO: does memory order matter here?
    if (auto message_id = _message_buffer.header()->message_id.published.load(std::memory_order_relaxed); message_id != _initial_message_info) {
      auto access = _message_buffer[message_id].acquire(_message_buffer);
      if (access.has_value()) {
        _initial_message_info = message_id;
        return std::move(access.value());
      }
    }
    return std::nullopt;
  }

  access_type await_message() {
    while (true) {
      // TODO: does memory order matter here?
      if (auto message_id = _message_buffer.header()->message_id.published.load(std::memory_order_relaxed); message_id != _initial_message_info) {
        auto access = _message_buffer[message_id].acquire();
        // TODO: check if access is valid and read newer if not
        if (access.has_value()) {
          _initial_message_info = message_id;
          return std::move(access.value());
        }
      }
    }
  }

 private:
  RealTimeSubscriber(Topic&& topic, const subscriber::Options& options, message_buffer<message_type>&& buffer)
      : _topic(std::move(topic)), _options(options), _message_buffer(std::move(buffer)) {}

 private:
  Topic _topic = nullptr;
  message_buffer<message_type> _message_buffer;
  ps::subscriber::Options _options;
  std::uint64_t _initial_message_info = std::numeric_limits<std::uint64_t>::max();
};

}  // namespace ipcpp::ps