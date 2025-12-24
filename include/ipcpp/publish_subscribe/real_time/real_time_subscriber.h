/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time/error_codes.h>
#include <ipcpp/publish_subscribe/real_time/real_time_memory_layout.h>
#include <ipcpp/publish_subscribe/real_time/real_time_message.h>
#include <ipcpp/topic.h>
#include <ipcpp/types.h>

#include <expected>
#include <optional>

namespace ipcpp::ps {

namespace internal {

std::expected<std::pair<uint_half_t, std::unique_ptr<utils::InterProcessLock>>, std::error_code>
get_new_subscriber_entry_idx(const std::string& topic_id, uint_half_t max_subscribers,
                             std::chrono::milliseconds timeout) {
  std::uint64_t start = utils::timestamp();
  while (true) {
    for (uint_half_t idx = 0; idx < max_subscribers; ++idx) {
      std::string name = std::format("{}_subscriber_entry_{}", topic_id, idx);
      auto lock = std::make_unique<utils::InterProcessLock>(name);
      bool acquired = false;
      if (lock->try_lock(acquired).value() == 0 && acquired) {
        return std::make_pair(idx, std::move(lock));
      }
    }
    if (start - utils::timestamp() >= std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count()) {
      return std::unexpected(std::make_error_code(std::errc::timed_out));
    }
  }
}

}  // namespace internal

template <typename T_p>
class RealTimeSubscriber {
 public:
  typedef T_p value_type;
  typedef rt::Message<T_p> message_type;
  typedef typename message_type::Access access_type;

 public:
  class MessageWrapper {
   public:
    MessageWrapper(std::atomic<int_t>* acquire_counter, access_type&& access)
        : _acquire_counter(acquire_counter), _access(std::move(access)) {}
    ~MessageWrapper() {
      cleanup();
    }

    void cleanup() {
      if (_acquire_counter == nullptr) [[unlikely]] /*only true if move constructor was used*/ {
        return;
      }
      _access.release();
      _acquire_counter->fetch_add(1, std::memory_order_release);
    }

    MessageWrapper(const MessageWrapper&) = delete;
    MessageWrapper(MessageWrapper&& other) : _access(std::move(other._access)) {
      std::swap(_acquire_counter, other._acquire_counter);
    }
    MessageWrapper& operator=(const MessageWrapper&) = delete;
    MessageWrapper& operator=(MessageWrapper&& other) {
      if (this != &other) {
        cleanup();
        _access = std::move(other._access);
        std::swap(_acquire_counter, other._acquire_counter);
      }
      return *this;
    }

    // provide direct access to the data hold by the access to avoid double dereferencing needs
    T_p* operator->() { return _access.operator->(); }
    const T_p* operator->() const { return _access->operator->(); }

    T_p& operator*() { return *_access; }
    const T_p& operator*() const { return *_access; }

    explicit operator bool() const { return bool(_access); }

   private:
    std::atomic<int_t>* _acquire_counter = nullptr;
    access_type _access;
  };

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

    uint_half_t subscriber_id = e_buffer->common_header()->next_subscriber_id.fetch_add(1, std::memory_order_release);
    uint_half_t max_concurrent_acquires = e_buffer->common_header()->options.max_concurrent_acquires;

    auto& topic = e_topic.value();
    auto& buffer = e_buffer.value();

    if (auto e = internal::get_new_subscriber_entry_idx(topic->id(), buffer.common_header()->options.max_subscribers,
                                                        1000ms);
        !e.has_value()) {
      return std::unexpected(e.error());
    } else {
      auto [_entry_idx, _entry_lock] = std::move(e.value());
      auto next_message_id = buffer.common_header()->next_message_id.load(std::memory_order_acquire);
      RealTimeSubscriber self(std::move(e_topic.value()), std::move(e_buffer.value()), subscriber_id,
                              max_concurrent_acquires, _entry_idx, std::move(_entry_lock), next_message_id);

      return self;
    }
  }

  /*
  ~RealTimeSubscriber() {
    // clear subscriber id entry
  }
   // TODO: implement move constructor here
   */

 public:
  std::expected<MessageWrapper, std::error_code> fetch_message() {
    if (auto message_id = _message_buffer.common_header()->next_message_id.load(std::memory_order_acquire);
        message_id != _next_message_id) {
      uint_t message_idx = _message_buffer.common_header()->latest_published_idx;
      auto access = _message_buffer[_m_get_index_from_id(message_idx)].acquire_unsafe();
      if (access) {
        _next_message_id = message_id;
        auto available_acquires = _available_acquires->fetch_sub(1, std::memory_order_acquire);
        if (available_acquires <= 0) {
          _available_acquires->fetch_add(1, std::memory_order_acquire);
          // TODO: error is that too many messages are already acquired
          // return std::unexpected(real_time::error::Subscriber::AcquireLimitExceeded);
          return std::unexpected(std::make_error_code(std::errc::invalid_seek));
        }
        return MessageWrapper(_available_acquires.get(), std::move(access));
      }
    }
    // return std::unexpected(real_time::error::Subscriber::NoMessageAvailable);
    return std::unexpected(std::make_error_code(std::errc::no_message_available));
  }

  std::expected<MessageWrapper, std::error_code> await_get_message() {
    while (true) {
      if (auto e_message = fetch_message(); e_message.has_value()) {
        return std::move(e_message.value());
        //} else if (e_message.error() == real_time::error::Subscriber::AcquireLimitExceeded) {
      } else if (e_message.error() == std::errc::invalid_seek) {
        return std::unexpected(e_message.error());
      }
    }
  }

  /**
   * @brief This method will block forever if the limit of allowed acquires is reached and no acquires are released.
   * @return
   */
  MessageWrapper await_message() {
    while (true) {
      if (auto opt = fetch_message(); opt.has_value()) {
        return std::move(opt.value());
      }
    }
  }

 private:
  RealTimeSubscriber(std::shared_ptr<ShmRegistryEntry>&& topic, RealTimeMessageBuffer<message_type>&& buffer,
                     uint_half_t subscriber_id, uint_half_t max_concurrent_acquires, uint_half_t entry_idx,
                     std::unique_ptr<utils::InterProcessLock>&& entry_lock, uint_t last_message_id)
      : _topic(std::move(topic)),
        _message_buffer(std::move(buffer)),
        _subscriber_id(subscriber_id),
        _entry_idx(entry_idx),
        _entry_lock(std::move(entry_lock)),
        _available_acquires(std::make_unique<std::atomic<int_t>>(max_concurrent_acquires)),
        _next_message_id(last_message_id) {}

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
  const uint_half_t _subscriber_id;
  const uint_half_t _entry_idx;
  std::unique_ptr<utils::InterProcessLock> _entry_lock;
  std::unique_ptr<std::atomic<int_t>> _available_acquires;
  uint_t _next_message_id;
};

}  // namespace ipcpp::ps