/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/rt_shm_layout.h>
#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/types.h>
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
    auto e_topic = get_shm_entry(topic_id);
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer = RealTimeMessageBuffer<message_type>::read_at(e_topic.value()->shm().addr());
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    RealTimeSubscriber self(std::move(e_topic.value()), options, std::move(e_buffer.value()));

    return self;
  }

 public:
  bool subscribe() {
    auto count = _message_buffer.common_header()->num_subscribers.fetch_add(1);
    if (count >= _message_buffer.common_header()->max_subscribers) {
      _message_buffer.common_header()->num_subscribers.fetch_sub(1);
      return false;
    }
    _initial_message_info = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
    return true;
  }

  void unsubscribe() { _message_buffer.header()->num_subscribers.fetch_sub(1); }

  std::optional<access_type> fetch_message() {
    if (auto message_id = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
        message_id != _initial_message_info) {
      auto access = _message_buffer[_m_get_index_from_id(message_id)].acquire_unsafe();
      if (access) {
        _initial_message_info = message_id;
        return std::move(access.value());
      }
    }
    return std::nullopt;
  }

  access_type await_message() {
    while (true) {
      if (auto message_id = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
          message_id != _initial_message_info) {
        auto message_access = _message_buffer[_m_get_index_from_id(message_id)].acquire_unsafe();
        if (message_access) {
          _initial_message_info = message_id;
          return std::move(message_access);
        }
      }
    }
  }

 private:
  RealTimeSubscriber(std::shared_ptr<ShmRegistryEntry>&& topic, const subscriber::Options& options, RealTimeMessageBuffer<message_type>&& buffer)
      : _topic(std::move(topic)), _options(options), _message_buffer(std::move(buffer)) {}

 private:
  numeric::half_size_int<uint_t>::type _m_get_index_from_id(uint_t id) {
    return static_cast<numeric::half_size_int<uint_t>::type>(
        id >> std::numeric_limits<typename numeric::half_size_int<uint_t>::type>::digits);
  }

 private:
  std::shared_ptr<ShmRegistryEntry> _topic = nullptr;
  RealTimeMessageBuffer<message_type> _message_buffer;
  ps::subscriber::Options _options;
  uint_t _initial_message_info = std::numeric_limits<std::uint64_t>::max();
  uint_half_t subscriber_id;
};

}  // namespace ipcpp::ps