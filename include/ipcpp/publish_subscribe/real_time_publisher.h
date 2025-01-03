/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/publish_subscribe/real_time_shm_layout.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/topic.h>

#include <expected>
#include <string>

namespace ipcpp::ps {

template <typename T_p>
class RealTimePublisher {
 public:
  typedef rt::Message<T_p> message_type;
  typedef typename rt::Message<T_p>::template Access<AccessMode::READ> access_type;

 public:
  static std::expected<RealTimePublisher, std::error_code> create(const std::string& topic_id,
                                                                  const ps::publisher::Options& options = {}) {
    auto e_topic =
        get_topic(topic_id, ps::RealTimeMemLayout<message_type>::required_size_bytes(options.queue_capacity));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer =
        RealTimeMemLayout<message_type>::init_at(e_topic.value()->shm().addr(), e_topic.value()->shm().size());
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    RealTimePublisher self(std::move(e_topic.value()), options, std::move(e_buffer.value()));

    return self;
  }

 public:
 template <typename... T_Args>
 std::error_code publish(T_Args&&... args) {
   std::uint64_t index = _chunk_buffer.allocate();
   if (index == std::numeric_limits<std::uint64_t>::max()) {
     // no buffer available
     logging::debug("RealTimePublisher<'{}'>::publish: failed due to allocation error", _topic->id());
     return {1, std::system_category()};
   }
   message_type& message = _chunk_buffer[index];
   {
     auto writable = message.request_writable(_chunk_buffer);
     writable.emplace(index, 0,
                      std::forward<T_Args>(args)...);
   }
   logging::debug("RealTimePublisher<'{}'>::publish: emplaced message at index {}", _topic->id(), index);
   auto access = message.acquire(_chunk_buffer).value();
   _m_notify_subscribers(index);
   _prev_published_message = std::make_unique<access_type>(std::move(access));
   return {};
 }

 private:
  RealTimePublisher(Topic&& topic, const publisher::Options& options, RealTimeMemLayout<message_type>&& buffer)
      : _topic(std::move(topic)), _options(options), _chunk_buffer(std::move(buffer)) {}

 private:
  inline void _m_notify_subscribers(std::uint64_t index) {
    //TODO: pass message and set msg id and increase counter here
    _chunk_buffer.header()->latest_message_index.store(index, std::memory_order_release);
    _chunk_buffer.header()->next_message_id_counter.fetch_add(1);
    logging::debug("RealTimePublisher<'{}'>::publish: notified subscribers: index: {}", _topic->id(), index);
  }

 private:
  Topic _topic = nullptr;
  RealTimeMemLayout<message_type> _chunk_buffer;
  ps::publisher::Options _options;
  std::unique_ptr<access_type> _prev_published_message = nullptr;
};

}  // namespace ipcpp::ps