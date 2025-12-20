/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/publish_subscribe/rt_shm_layout.h>
#include <ipcpp/topic.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/utils.h>

#include <expected>
#include <string>
#include <filesystem>

namespace ipcpp::ps {

namespace internal {

std::pair<uint_half_t, utils::InterProcessLock> get_new_publisher_entry(const std::string& topic_id, uint_half_t max_publishers) {
  while (true) {
    for (uint_half_t idx = 0; idx < max_publishers; ++idx) {
      std::string name = std::format("{}_publisher_entry_{}", topic_id, idx);
      utils::InterProcessLock lock(name);
      bool acquired = false;
      if (lock.try_lock(acquired).value() == 0 && acquired) {
        return std::make_pair(idx, std::move(lock));
      }
    }
  }
}

}

template <typename T_p>
class RealTimePublisher {
 public:
  typedef rt::Message<T_p> message_type;
  typedef typename rt::Message<T_p>::Access access_type;

 public:
  static std::expected<RealTimePublisher, std::error_code> create(const std::string& topic_id,
                                                                  const ps::publisher::Options& options = {}) {
    auto e_topic = get_shm_entry(topic_id, RealTimeMessageBuffer<message_type>::required_size_bytes(
                                               options.max_num_observers, options.max_num_publishers));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_buffer = RealTimeMessageBuffer<message_type>::read_at(e_topic.value()->shm().addr());
    if (!e_buffer) {
      e_buffer =
          RealTimeMessageBuffer<message_type>::init_at(e_topic.value()->shm().addr(), e_topic.value()->shm().size(),
                                                       options.max_num_observers, options.max_num_publishers);
    }
    if (!e_buffer) {
      return std::unexpected(e_buffer.error());
    }

    RealTimeMessageBuffer<message_type>& buffer = e_buffer.value();
    uint_half_t publisher_id = buffer.common_header()->num_publishers.fetch_add(1);
    if (publisher_id >= buffer.common_header()->max_publishers) {
      // TODO: too many publishers
      buffer.common_header()->num_publishers.fetch_sub(1);
      return std::unexpected(std::error_code(2, std::system_category()));
    }

    // search free publisher_idx
    auto [idx, lock] = internal::get_new_publisher_entry(topic_id, buffer.common_header()->max_publishers);

    RealTimePublisher self(std::move(e_topic.value()), options, std::move(e_buffer.value()), publisher_id, idx, std::move(lock));

    return self;
  }

 public:
  template <typename... T_Args>
  std::error_code publish(T_Args&&... args) {
    uint_half_t local_message_id = _pp_header->next_local_message_id++;
    uint_half_t idx = local_message_id & _wrap_around_value;
    auto& message = _assigned_area[idx];
    auto message_id = _m_build_message_identifier(_publisher_id, local_message_id);
    message.emplace(message_id, std::forward<T_Args>(args)...);
    logging::debug("RealTimePublisher<'{}'>::publish: emplaced message #{} (publisher: {}, local_index: {})",
                   _topic->id(), message_id, _publisher_id, local_message_id);
    _m_notify_subscribers(message_id);
    _prev_published_message = std::move(message.acquire_unsafe());

    return {};
  }

 private:
  RealTimePublisher(std::shared_ptr<ShmRegistryEntry>&& topic, const publisher::Options& options,
                    RealTimeMessageBuffer<message_type>&& buffer, uint_half_t publisher_id, uint_half_t entry_idx, utils::InterProcessLock&& lock)
      : _topic(std::move(topic)), _options(options), _message_buffer(std::move(buffer)), _publisher_id(publisher_id), _entry_idx(entry_idx), _entry_lock(std::move(lock)) {
    _assigned_area = std::span<message_type>(&_message_buffer[_message_buffer.get_index(_entry_idx, 0)],
                                             _message_buffer.per_publisher_pool_size(_message_buffer.common_header()->max_subscribers));
    _wrap_around_value = _assigned_area.size() - 1;
    _pp_header = _message_buffer.per_publisher_header(_entry_idx);
    std::construct_at(_pp_header, _entry_idx);
  }

 private:
  inline void _m_notify_subscribers(uint_t message_id) {
    _message_buffer.common_header()->latest_published.store(message_id, std::memory_order_relaxed);
    logging::debug("RealTimePublisher<'{}'>::publish: notified subscribers about published message #{}", _topic->id(),
                   message_id);
  }

  inline uint_t _m_build_message_identifier(uint_half_t index, uint_half_t local_message_id) {
    return (static_cast<uint_t>(index) << std::numeric_limits<uint_half_t>::digits) |
           (static_cast<uint_half_t>(local_message_id));
  }

 private:
  /// the full shared memory entry to ensure its validity
  std::shared_ptr<ShmRegistryEntry> _topic = nullptr;
  /// the full MessageBuffer part
  RealTimeMessageBuffer<message_type> _message_buffer;
  /// the message buffer area assigned to this publisher
  std::span<message_type> _assigned_area;
  /// this publishers header
  RealTimePublisherEntry* _pp_header = nullptr;
  /// options
  ps::publisher::Options _options;
  /// id
  uint_half_t _publisher_id;
  /// book keeping to avoid immediate destruction
  access_type _prev_published_message;
  /// wrap around value for fast modulo
  uint_half_t _wrap_around_value;
  /// publisher entry idx in shm
  uint_half_t _entry_idx;
  /// lock for the entry idx
  utils::InterProcessLock _entry_lock;
};

}  // namespace ipcpp::ps