/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notifier.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/publish_subscribe/error.h>
#include <ipcpp/publish_subscribe/fifo_message.h>
#include <ipcpp/publish_subscribe/fifo_message_queue.h>
#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/topic.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/numeric.h>

#include <concepts>
#include <optional>
#include <utility>

namespace ipcpp::publish_subscribe {

namespace internal {

struct ShmDefaultNotifier {};

}  // namespace internal

template <typename T_Data, typename T_Notifier = internal::ShmDefaultNotifier>
class Publisher final {
 public:
  typedef T_Data data_type;
  typedef ps::Message<T_Data> data_access_type;
  typedef T_Notifier notifier_type;

 public:
  Publisher(const Publisher& other) = delete;
  Publisher(Publisher&&) = default;

 public:
  static std::expected<Publisher, std::error_code> create(const std::string& topic_id,
                                                          const ps::publisher::Options& options) {
    auto e_topic = get_shm_entry(topic_id, numeric::ceil_to_power_of_two(options.queue_capacity * sizeof(T_Data)));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_message_queue =
        ps::shm_message_queue<data_access_type>::init_at(e_topic.value()->shm().addr(), e_topic.value()->shm().size());
    if (!e_message_queue) {
      return std::unexpected(e_message_queue.error());
    }
    std::string notifier_topic_id(topic_id + "_notifications");
    auto e_notifier = T_Notifier::create(notifier_topic_id);
    if (!e_notifier) {
      return std::unexpected(e_notifier.error());
    }

    Publisher self(std::move(e_topic.value()), options);
    self._message_queue = std::make_unique<ps::shm_message_queue>(std::move(e_message_queue.value()));
    self._notifier = std::make_unique<T_Notifier>(std::move(e_notifier.value()));

    return self;
  }

  /*
  std::error_code publish(data_access_type* data) {
    switch (this->_options.publish_policy) {
      case publisher::PublishPolicy::always: {
        const std::size_t index = _message_buffer->get_index(data);
        _m_notify_observers(index);
        logging::debug("Publisher<'{}'>::publish(): published message at {}", this->_topic->id(), index);
        return {static_cast<int>(publisher::error_t::success), publisher::error_category()};
      }
      case publisher::PublishPolicy::subscribed: {
        if (_m_num_observers() > 0) {
          const std::size_t index = _message_buffer->get_index(data);
          _m_notify_observers(index);
          logging::debug("Publisher<'{}'>::publish(): published message at {}", this->_topic->id(), index);
          return {static_cast<int>(publisher::error_t::success), publisher::error_category()};
        }
        return {static_cast<int>(publisher::error_t::no_active_subscription), publisher::error_category()};
      }
      case publisher::PublishPolicy::timed: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::timed", this->_topic->id());
        return {static_cast<int>(publisher::error_t::publish_rate_limit_exceeded), publisher::error_category()};
      }
      default:
        std::unreachable();  // set_config/set_publish_policy must handle invalid policy settings
    }
  }

  template <typename... T_Args>
  locked_write_access_type emplace(T_Args&&... args) {
    logging::info("Publisher<'{}'>::emplace(): constructed message", this->_topic->id());
    auto write_access_value =
  _message_queue->operator[](_message_queue->header()->message_id.next.fetch_add(1)).write_access();
    write_access_value.emplace(-1, std::forward<T_Args>(args)...);
    return write_access_value;
  }
   */

  template <typename... T_Args>
  std::error_code publish(T_Args&&... args) {
    const std::size_t index = _message_queue->header()->message_id.next.load(std::memory_order_acquire);
    bool published = false;
    for (std::size_t i = 0; i < _m_num_observers(); ++i) {
      // TODO: o_access must be released before the notifier sends anything!
      auto o_access = _message_queue->operator[](index).write_access();
      if (o_access) {
        o_access->emplace(_m_num_observers(), std::forward<T_Args>(args)...);
        _notifier->notify_observers(i + 1);
        _message_queue->header()->message_id.next.fetch_add(i + 1);
        published = true;
        break;
      }
    }
    return {static_cast<int>((published ? publisher::error_t::success : publisher::error_t::unknown_error)),
            publisher::error_category()};
  }

 private:
  Publisher(Topic&& topic, const ps::publisher::Options& options) : _topic(std::move(topic)), _options(options) {}

 private:
  std::error_code _m_initialize_notifier() {
    auto expected_notifier = notifier_type::create(std::string(this->_topic->id()), 4096);
    if (!expected_notifier.has_value()) {
      // TODO: return error
      return {};
    }
    _notifier = std::make_unique<notifier_type>(std::move(expected_notifier.value()));
    logging::info("Publisher<'{}'>::_m_initialize_notifier(): created notifier", this->_topic->id());
    return {};
  }

  [[nodiscard]] std::size_t _m_num_observers() const { return _notifier->num_observers(); }

  void _m_notify_observers(std::size_t index) { _notifier->notify_observers(index); }

 private:
  Topic _topic = nullptr;
  std::unique_ptr<ps::shm_message_queue<data_access_type>> _message_queue = nullptr;
  ps::publisher::Options _options;
  std::unique_ptr<notifier_type> _notifier = nullptr;
};

/**
 * Specialization for default shm publisher
 */
template <typename T_Data>
class Publisher<T_Data, internal::ShmDefaultNotifier> final {
 public:
  typedef T_Data data_type;
  typedef ps::Message<T_Data> data_access_type;

 public:
  static std::expected<Publisher, std::error_code> create(const std::string& topic_id,
                                                          const ps::publisher::Options& options = {}) {
    auto e_topic = get_shm_entry(
        topic_id, numeric::ceil_to_power_of_two(
                      ps::shm_message_queue<data_access_type>::required_size_bytes(options.queue_capacity)));
    if (!e_topic) {
      return std::unexpected(e_topic.error());
    }
    auto e_message_queue =
        ps::shm_message_queue<data_access_type>::init_at(e_topic.value()->shm().addr(), e_topic.value()->shm().size());
    if (!e_message_queue) {
      return std::unexpected(e_message_queue.error());
    }

    Publisher self(std::move(e_topic.value()), options);
    self._message_queue = std::make_unique<ps::shm_message_queue<data_access_type>>(std::move(e_message_queue.value()));

    return self;
  }

  /*
  std::error_code publish(data_access_type* data) {
    assert(_initialized);
    std::error_code error(static_cast<int>(publisher::error_t::unknown_error), publisher::error_category());
    switch (this->_options.publish_policy) {
      case publisher::PublishPolicy::always: {
        const std::size_t msg_id = data->message_number;
        _m_notify_observers(msg_id);
        logging::debug("Publisher<'{}'>::publish(): published message id {}", this->_topic->id(), msg_id);
        error = {};
        break;
      }
      case publisher::PublishPolicy::subscribed: {
        if (_m_num_observers() > 0) {
          const std::size_t msg_id = data->message_number;
          _m_notify_observers(msg_id);
          logging::debug("Publisher<'{}'>::publish(): published message id {}", this->_topic->id(), msg_id);
          error = {};
        } else {
          error = std::error_code(static_cast<int>(publisher::error_t::no_active_subscription),
                                  publisher::error_category());
        }
        break;
      }
      case publisher::PublishPolicy::timed: {
        if (_last_publish_timestamp == 0 ||
            utils::timestamp() - _last_publish_timestamp > _options.timed_publish_interval.count()) {
          const std::size_t msg_id = data->message_number;
          _m_notify_observers(msg_id);
          logging::debug("Publisher<'{}'>::publish(): published message id {}", this->_topic->id(), msg_id);
          error = {};
        } else {
          error = std::error_code(static_cast<int>(publisher::error_t::publish_rate_limit_exceeded),
                                  publisher::error_category());
        }
        break;
      }
      default:
        std::unreachable();  // set_config/set_publish_policy must handle invalid policy settings
    }
    if (!error) {
      _last_publish_timestamp = utils::timestamp();
    }
    return error;
  }

  template <typename... T_Args>
  data_access_type* construct_and_get(T_Args&&... args) {
    assert(_initialized);
    logging::debug("Publisher<'{}'>::construct_and_get(): constructed next message", this->_topic->id());
    std::size_t msg_id = _message_buffer->header->message_counter.load(std::memory_order_acquire) + 1;
    _m_handle_backpressure(msg_id);
    return _message_buffer->message_buffer.emplace_at(msg_id, std::forward<T_Args>(args)...,
                                                      _m_num_observers() + (this->_options.history_capacity > 0));
  }
  */

  template <typename... T_Args>
  std::error_code publish(T_Args&&... args) {
    if (_m_publish(std::forward<T_Args>(args)...)) {
      return {};
    }
    return std::error_code(static_cast<int>(publisher::error_t::unknown_error), publisher::error_category());
  }

 private:
  Publisher(Topic&& topic, const ps::publisher::Options& options) : _topic(std::move(topic)), _options(options) {}

  [[nodiscard]] std::size_t _m_num_observers() const {
    return _message_queue->header()->num_subscribers.load(std::memory_order_acquire);
  }

  void _m_notify_observers(std::size_t index) {
    _message_queue->header()->message_id.next.store(index, std::memory_order_release);
    logging::info("Publisher<'{}'>::_m_notify_observers(): message_id: {}", this->_topic->id(), index);
  }

  std::uint64_t _m_handle_backpressure(std::uint64_t msg_id) {
    auto& message_chunk = _message_queue->operator[](msg_id);
    if (message_chunk.message_id() == std::numeric_limits<std::uint64_t>::max()) {
      // no backpressure, chunk was never used before
      logging::debug("Publisher<'{}'>::_m_handle_backpressure: chunk was never used", _topic->id());
      return msg_id;
    } else {
      std::uint64_t remaining_references = message_chunk.remaining_references();
      logging::debug("Publisher<'{}'>::_m_handle_backpressure: remaining references: {}", _topic->id(),
                     remaining_references);
      if (remaining_references <= 0) {
        // no backpressure, chunk was read by all subscribers
        logging::debug("Publisher<'{}'>::_m_handle_backpressure: chunk is free", _topic->id());
        return msg_id;
      } else {
        logging::info("Publisher<'{}'>::publish(): Backpressure for message id: {}", this->_topic->id(), msg_id);
        switch (_options.backpressure_policy) {
          case ps::publisher::BackpressurePolicy::block: {
            const std::size_t history_size = this->_options.history_capacity;
            remaining_references = message_chunk.remaining_references();
            while (true) {
              if (remaining_references <= 0) {
                break;
              }
              std::this_thread::yield();
              remaining_references = message_chunk.remaining_references();
            }
            return msg_id;
          }
          case ps::publisher::BackpressurePolicy::remove_oldest: {
            logging::debug(
                "Publisher<'{}'>::publish(): resetting message #{} as part of backpressure strategy (remove_oldest)",
                this->_topic->id(), msg_id);
            for (std::size_t i = 0; i < _message_queue->size(); ++i) {
              if (auto access = _message_queue->operator[](msg_id + i).request_writable(); access) {
                access.value().reset();
                logging::debug("Publisher<'{}'>::publish(): message #{} reset", this->_topic->id(), msg_id + i);
                return msg_id + i;
              }
            }
            logging::debug("Publisher<'{}'>::publish(): failed to handle backpressure", this->_topic->id());
            return std::numeric_limits<std::uint64_t>::max();
          }
          default:
            std::unreachable();
        }
      }
    }
  }

  template <typename... T_Args>
  bool _m_publish(T_Args&&... args) {
    auto msg_id = _message_queue->header()->message_id.next.load(std::memory_order_acquire);
    // msg_id = _m_handle_backpressure(msg_id);
    bool published = false;
    if (auto o_access = _message_queue->operator[](msg_id).request_writable(); o_access) {
      o_access->emplace(_m_num_observers(), msg_id, std::forward<T_Args>(args)...);
      published = true;
    }
    if (published) {
      // o_access must be released at this point
      _m_notify_observers(msg_id + 1);
      logging::debug("Publisher<'{}'>::publish(): published message (#{}) at {}", _topic->id(), msg_id, msg_id);
      return true;
    }
    return false;
  }

 private:
  Topic _topic = nullptr;
  std::unique_ptr<ps::shm_message_queue<data_access_type>> _message_queue = nullptr;
  ps::publisher::Options _options;
};

}  // namespace ipcpp::publish_subscribe