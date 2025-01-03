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
    _initial_message_id = _chunk_buffer.header()->next_message_id_counter.load(std::memory_order_acquire);
  }

  void unsubscribe() { _chunk_buffer.header()->num_subscribers.fetch_sub(1); }

  std::optional<access_type> fetch_message() {
    if (std::uint64_t new_message_id = _chunk_buffer.header()->next_message_id_counter.load(std::memory_order_acquire); new_message_id != _initial_message_id) {
      _initial_message_id = new_message_id;
      // TODO: replace magic number 10 for retries
      message_type* message = nullptr;
      while (true) {
        std::uint64_t index = _chunk_buffer.header()->latest_message_index.load(std::memory_order_acquire);
        message = &_chunk_buffer[index];
        if (auto access = message->acquire(_chunk_buffer, 10); access) {
          return std::move(access.value());
        }
      }
    }
    return std::nullopt;
  }

  std::expected<access_type, std::error_code> await_message() {
    std::uint64_t new_message_id = 0;
    while (true) {
      new_message_id = _chunk_buffer.header()->next_message_id_counter.load(std::memory_order_acquire);
      if (new_message_id != _initial_message_id) {
        break;
      }
    }
    _initial_message_id = new_message_id;
    // TODO: replace magic number 10 for retries
    logging::debug("RealTimeSubscriber<'{}'>::await_message: reading latest message: index {}", _topic->id(),
                   _chunk_buffer.header()->latest_message_index.load(std::memory_order_acquire));
    message_type* message = nullptr;
    while (true) {
      std::uint64_t index = _chunk_buffer.header()->latest_message_index.load(std::memory_order_acquire);
      logging::debug("RealTimeSubscriber<'{}'>::await_message: reading latest message: index {}", _topic->id(), index);
      message = &_chunk_buffer[index];
      if (auto access = message->acquire(_chunk_buffer); access) {
        return std::move(access.value());
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
  std::uint64_t _initial_message_id = std::numeric_limits<std::uint64_t>::max();
};

}  // namespace ipcpp::ps