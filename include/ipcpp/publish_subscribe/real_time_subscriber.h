/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/publish_subscribe/real_time_shm_layout.h>
#include <ipcpp/topic.h>

#include <optional>

namespace ipcpp::ps {

template <typename T_p>
class RealTimeSubscriber {
 public:
  typedef rt::Message<T_p> message_type;
  typedef typename message_type::template Access<AccessMode::READ> access_type;

 public:
  static std::expected<RealTimeSubscriber, std::error_code> create(const std::string& topic_id,
                                                                   const subscriber::Options& options = {}) {
    auto e_topic = get_topic(topic_id);
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer = RealTimeMemLayout<message_type>::read_at(e_topic.value()->shm().addr());
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    RealTimeSubscriber self(std::move(e_topic.value()), options, std::move(e_buffer.value()));

    return self;
  }

 public:
  void subscribe() {
    _chunk_buffer.header()->num_subscribers.fetch_add(1);
    _initial_message_info = _chunk_buffer.header()->message_info.load(std::memory_order_acquire);
  }

  void unsubscribe() { _chunk_buffer.header()->num_subscribers.fetch_sub(1); }

  std::optional<access_type> fetch_message() {
    if (std::uint64_t message_info = _chunk_buffer.header()->message_info.load(std::memory_order_acquire); message_info != _initial_message_info) {
      auto message_index = static_cast<std::uint32_t>(message_info >> 32);
      auto access = _chunk_buffer[message_index].acquire(_chunk_buffer);
      // TODO: check if access is valid and read newer if not
      if (access.has_value()) {
        _initial_message_info = message_info;
        return std::move(access.value());
      }
    }
    return std::nullopt;
  }

  std::expected<access_type, std::error_code> await_message() {
    while (true) {
      if (std::uint64_t message_info = _chunk_buffer.header()->message_info.load(std::memory_order_acquire); message_info != _initial_message_info) {
        auto message_index = static_cast<std::uint32_t>(message_info >> 32);
        auto access = _chunk_buffer[message_index].acquire(_chunk_buffer);
        // TODO: check if access is valid and read newer if not
        if (access.has_value()) {
          _initial_message_info = message_info;
          return std::move(access.value());
        }
      }
    }
  }

 private:
  RealTimeSubscriber(Topic&& topic, const subscriber::Options& options, RealTimeMemLayout<message_type>&& buffer)
      : _topic(std::move(topic)), _options(options), _chunk_buffer(std::move(buffer)) {}

 private:
  Topic _topic = nullptr;
  RealTimeMemLayout<message_type> _chunk_buffer;
  ps::subscriber::Options _options;
  std::uint64_t _initial_message_info = std::numeric_limits<std::uint64_t>::max();
};

}  // namespace ipcpp::ps