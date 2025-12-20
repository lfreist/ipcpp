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
#include <expected>

namespace ipcpp::ps {

namespace internal {

std::expected<std::pair<uint_half_t, std::unique_ptr<utils::InterProcessLock>>, std::error_code> get_new_subscriber_entry_idx(const std::string& topic_id, uint_half_t max_subscribers) {
  for (int retries = 0; retries < 100; ++retries) {
    for (uint_half_t idx = 0; idx < max_subscribers; ++idx) {
      std::string name = std::format("{}_subscriber_entry_{}", topic_id, idx);
      auto lock = std::make_unique<utils::InterProcessLock>(name);
      bool acquired = false;
      if (lock->try_lock(acquired).value() == 0 && acquired) {
        return std::make_pair(idx, std::move(lock));
      }
    }
  }
  return std::unexpected(std::make_error_code(std::errc::no_buffer_space));
}

}

// TODO: maybe, it would be clean to create a RealTimeSubscriberStream that can read from the shm and on unsubscribe, we invalidate the stream...

template <typename T_p>
class RealTimeSubscriber {
public:
 typedef rt::Message<T_p> message_type;
 typedef typename message_type::Access access_type;

public:
 static std::expected<RealTimeSubscriber, std::error_code> create(const std::string& topic_id) {
   auto e_topic = get_shm_entry(topic_id);
   if (!e_topic) {
     return std::unexpected(e_topic.error());
   }
   auto e_buffer = RealTimeMessageBuffer<message_type>::read_at(e_topic.value()->shm().addr());
   if (!e_buffer) {
     return std::unexpected(e_buffer.error());
   }

   RealTimeSubscriber self(std::move(e_topic.value()), std::move(e_buffer.value()));

   return self;
 }

public:
 std::expected<void, std::error_code> subscribe() {
   if (auto e = internal::get_new_subscriber_entry_idx(_topic->id(), _message_buffer.common_header()->max_subscribers); e.has_value()) {
     std::tie(_entry_idx, _entry_lock) = std::move(e.value());
     _initial_message_info = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
     return {};
   } else {
     return std::unexpected(e.error());
   }
 }


 void unsubscribe() {
     _entry_idx = std::numeric_limits<uint_half_t>::max();
     _entry_lock.reset(nullptr);
 }

 std::optional<access_type> fetch_message() {
   if (auto message_id = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
       message_id != _initial_message_info) {
     auto access = _message_buffer[_m_get_index_from_id(message_id)].acquire_unsafe();
     if (access) {
       _initial_message_info = message_id;
       return std::move(access);
     }
   }
   return std::nullopt;
 }

 access_type await_message() {
   while (true) {
     if (auto message_id = _message_buffer.common_header()->latest_published.load(std::memory_order_acquire);
         message_id != _initial_message_info) {
       auto [publisher_id, local_id] = _m_split_to_indices(message_id);
       auto idx = _message_buffer.get_index(publisher_id, local_id);
       auto message_access = _message_buffer[idx].acquire_unsafe();
       if (message_access) {
         _initial_message_info = message_id;
         return std::move(message_access);
       }
     }
   }
 }

private:
 RealTimeSubscriber(std::shared_ptr<ShmRegistryEntry>&& topic, RealTimeMessageBuffer<message_type>&& buffer)
     : _topic(std::move(topic)), _message_buffer(std::move(buffer)) {}

private:
 inline uint_half_t _m_get_index_from_id(uint_t message_id) {
   auto [publisher_id, local_id] = _m_split_to_indices(message_id);
   return _message_buffer.get_index(publisher_id, local_id);
 }

 inline std::pair<uint_half_t, uint_half_t> _m_split_to_indices(uint_t message_id) {
   return {message_id >> std::numeric_limits<uint_half_t>::digits, static_cast<uint_half_t>(message_id)};
 }

private:
 std::shared_ptr<ShmRegistryEntry> _topic = nullptr;
 RealTimeSubscriberEntry* _subscriber_entry = nullptr;
 RealTimeMessageBuffer<message_type> _message_buffer;
 uint_t _initial_message_info = std::numeric_limits<uint_t>::max();
 uint_half_t _subscriber_id = std::numeric_limits<uint_half_t>::max();
 uint_half_t _entry_idx = std::numeric_limits<uint_half_t>::max();
 std::unique_ptr<utils::InterProcessLock> _entry_lock = nullptr;
};

}  // namespace ipcpp::ps