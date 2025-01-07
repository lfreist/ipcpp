/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/message_buffer.h>
#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/types.h>
#include <ipcpp/topic.h>
#include <ipcpp/utils/numeric.h>

#include <expected>
#include <string>

namespace ipcpp::ps {

template <typename T_p>
class RealTimePublisher {
 public:
  typedef rt::Message<T_p> message_type;
  typedef typename rt::Message<T_p>::Access access_type;

 public:
  static std::expected<RealTimePublisher, std::error_code> create(const std::string& topic_id,
                                                                  const ps::publisher::Options& options = {}) {
    bool multi_publisher = options.max_num_publishers > 1;
    auto e_topic = get_shm(topic_id, message_buffer<message_type>::required_size_bytes(options.max_num_observers,
                                                                                       options.max_num_publishers));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer = message_buffer<message_type>::read_at(e_topic.value()->shm().addr());
    if (!e_buffer) {
      e_buffer = message_buffer<message_type>::init_at(e_topic.value()->shm().addr(), e_topic.value()->shm().size(),
                                                       options.max_num_publishers, options.max_num_observers);
    }
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    message_buffer<message_type>& buffer = e_buffer.value();
    auto publisher_id = buffer.common_header()->num_publishers.fetch_add(1);
    if (publisher_id >= buffer.common_header()->max_publishers) {
      // TODO: too many publishers
      buffer.common_header()->num_publishers.fetch_sub(1);
      return std::unexpected(std::error_code(2, std::system_category()));
    }

    RealTimePublisher self(std::move(e_topic.value()), options, std::move(e_buffer.value()), publisher_id);

    return self;
  }

 public:
  //~RealTimePublisher() {}

  // RealTimePublisher(RealTimePublisher&& other) : _message_buffer(std::move(other._message_buffer)),
  // _prev_published_message(std::move(other._prev_published_message)), _options(std::move(other._options)),
  // _topic(std::move(other._topic)) {}

  template <typename... T_Args>
  std::error_code publish(T_Args&&... args) {
    auto local_message_id = _pp_header->next_local_message_id++;
    auto index = _message_buffer.get_index(_publisher_id, local_message_id);
    auto& message = _message_buffer[index];
    auto message_id = _m_build_message_identifier(_publisher_id, local_message_id);
    message.emplace(message_id, std::forward<T_Args>(args)...);
    logging::debug("RealTimePublisher<'{}'>::publish: emplaced message #{} (index: {})", _topic->id(), message_id,
                   index);
    _m_notify_subscribers(message_id);
    _prev_published_message = std::move(message.acquire_unsafe());
    return {};
  }

 private:
  RealTimePublisher(Topic&& topic, const publisher::Options& options, message_buffer<message_type>&& buffer,
                    uint_half_t publisher_id)
      : _topic(std::move(topic)), _options(options), _message_buffer(std::move(buffer)), _publisher_id(publisher_id) {
    _pp_header = _message_buffer.per_publisher_header(_publisher_id);
  }

 private:
  inline void _m_notify_subscribers(uint_t message_id) {
    _message_buffer.common_header()->latest_published.store(message_id, std::memory_order_relaxed);
    logging::debug("RealTimePublisher<'{}'>::publish: notified subscribers about published message #{}", _topic->id(),
                   message_id);
  }

  inline uint_t _m_build_message_identifier(uint_t index, uint_t local_message_id) {
    return (index << std::numeric_limits<numeric::half_size_int<uint_t>>::digits) |
           (static_cast<numeric::half_size_int<uint_t>::type>(local_message_id));
  }

 private:
  Topic _topic = nullptr;
  message_buffer<message_type> _message_buffer;
  message_buffer<message_type>::PerPublisherHeader* _pp_header = nullptr;
  ps::publisher::Options _options;
  access_type _prev_published_message;
  uint_half_t _publisher_id;
};

}  // namespace ipcpp::ps