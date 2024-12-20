/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notifier.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/reference_counted.h>

#include <concepts>
#include <optional>

namespace ipcpp::publish_subscribe {

namespace internal {

template <typename...>
struct ShmDefault {};

template <typename T_Data>
class Publisher_I {
 public:
  explicit Publisher_I(std::string&& id) : _id(std::move(id)) {}
  Publisher_I(std::string&& id, const publisher::Options& options) : _id(std::move(id)), _options(options) {}
  virtual ~Publisher_I() = default;

  Publisher_I& set_options(const publisher::Options& options) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_options(): called but Publisher already set up", _id);
    } else {
      _options = options;
    }
    return *this;
  }

  Publisher_I& set_queue_capacity(const std::size_t num_elements) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_queue_capacity(): called but Publisher already set up", _id);
    } else {
      _options.queue_capacity = num_elements;
    }
    return *this;
  }

  Publisher_I& set_history_capacity(const std::size_t num_elements) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_history_capacity(): called but Publisher already set up", _id);
    } else {
      _options.history_capacity = num_elements;
    }
    return *this;
  }

  Publisher_I& set_backpressure_policy(const publisher::BackpressurePolicy policy) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_backpressure_policy(): called but Publisher already set up", _id);
    } else {
      _options.backpressure_policy = policy;
    }
    return *this;
  }

  Publisher_I& set_publish_policy(const publisher::PublishPolicy policy,
                                  std::chrono::milliseconds timed_publish_interval = 0ms) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_publish_policy(): called but Publisher already set up", _id);
    } else {
      if (policy == publisher::PublishPolicy::TIMED && timed_publish_interval.count() == 0) {
        logging::warn(
            "Publisher<'{}'>::set_publish_policy(PublishPolicy::TIMED, {}): converted to PublishPolicy::ALWAYS because "
            "interval value is 0",
            _id, timed_publish_interval);
        _options.publish_policy = publisher::PublishPolicy::ALWAYS;
      } else {
        _options.publish_policy = policy;
        _options.timed_publish_interval = timed_publish_interval;
      }
    }
    return *this;
  }

  Publisher_I& set_max_num_observers(const std::size_t num_observers) & {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::set_backpressure_policy(): called but Publisher already set up", _id);
    } else {
      _options.max_num_observers = num_observers;
    }
    return *this;
  }

  virtual std::error_code initialize() = 0;

 protected:
  [[nodiscard]] virtual std::size_t _m_num_observers() const = 0;

 protected:
  std::string _id;
  publisher::Options _options{};
  bool _initialized = false;
};

}  // namespace internal

template <typename T_Data, template <typename...> typename T_Notifier = internal::ShmDefault>
  requires(event::concepts::is_notifier<T_Notifier<std::size_t>> ||
           std::is_same_v<T_Notifier<std::size_t>, internal::ShmDefault<std::size_t>>)
class Publisher final : public internal::Publisher_I<T_Data> {
 public:
  typedef T_Data data_type;
  typedef reference_counted<T_Data> data_access_type;
  typedef T_Notifier<std::size_t> notifier_type;
  typedef typename T_Notifier<std::size_t>::notification_type notification_type;

 public:
  using internal::Publisher_I<T_Data>::Publisher_I;

  std::error_code initialize() override {
    assert(!this->_initialized);
    if (this->_initialized) {
      logging::warn("Publisher<'{}'>::initialize(): called but already set up", this->_id);
      return {};
    }
    if (const std::error_code error = _m_initialize_message_buffer()) {
      logging::error("Publisher<'{}'>::initialize(): failed to initialize message buffer: {}::{}", this->_id,
                     error.category().name(), error.message(), error.message());
      return error;
    }
    if (const std::error_code error = _m_initialize_notifier()) {
      logging::error("Publisher<'{}'>::initialize(): failed to initialize notifier: {}::{}", this->_id, error.category().name(),
                     error.message(), error.message());
      return error;
    }
    this->_initialized = true;
    logging::debug("Publisher<'{}'>::initialize(): initialization done", this->_id);
    return {};
  }

  void publish(data_access_type* data) {
    assert(this->_initialized);
    switch (this->_options.publish_policy) {
      case publisher::PublishPolicy::ALWAYS: {
        const std::size_t index = _message_buffer->get_index(data);
        _m_notify_observers(index);
        logging::info("Publisher<'{}'>::publish(): published message at {}", this->_id, index);
        break;
      }
      case publisher::PublishPolicy::SUBSCRIBED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::SUBSCRIBED", this->_id);
        break;
      }
      case publisher::PublishPolicy::TIMED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::TIMED", this->_id);
        break;
      }
    }
  }

  template <typename... T_Args>
  data_access_type* construct_and_get(T_Args&&... args) {
    assert(this->_initialized);
    logging::info("Publisher<'{}'>::construct_and_get(): constructed next message", this->_id);
    return _message_buffer->emplace(T_Data(std::forward<T_Args>(args)...),
                                    _m_num_observers() + (this->_options.history_capacity > 0));
  }

  template <typename... T_Args>
  void publish(T_Args&&... args) {
    assert(this->_initialized);
    switch (this->_options.publish_policy) {
      case publisher::PublishPolicy::ALWAYS: {
        const std::size_t index = _message_buffer->get_index(_message_buffer->emplace(
            T_Data(std::forward<T_Args>(args)...), _m_num_observers() + (this->_options.history_capacity > 0)));
        _m_notify_observers(index);
        logging::info("Publisher<'{}'>::publish(): published message at {}", this->_id, index);
        break;
      }
      case publisher::PublishPolicy::SUBSCRIBED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::SUBSCRIBED", this->_id);
        break;
      }
      case publisher::PublishPolicy::TIMED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::TIMED", this->_id);
        break;
      }
    }
  }

private:
  std::error_code _m_initialize_notifier() {
    auto expected_notifier = notifier_type::create(std::string(this->_id), 4096);
    if (!expected_notifier.has_value()) {
      // TODO: return error
      return {};
    }
    _notifier = std::make_unique<notifier_type>(std::move(expected_notifier.value()));
    logging::info("Publisher<'{}'>::_m_initialize_notifier(): created notifier", this->_id);
    return {};
  }

  std::error_code _m_initialize_message_buffer() {
    const std::size_t num_bytes =
        shm::ring_buffer<reference_counted<T_Data>>::required_bytes_for(this->_options.queue_capacity);
    auto expected_memory =
        shm::MappedMemory<shm::MappingType::SINGLE>::open_or_create("/" + this->_id + ".ipcpp.mrb.shm", num_bytes);
    if (!expected_memory.has_value()) {
      // TODO: return error
      return {};
    }
    _message_memory =
        std::make_optional<shm::MappedMemory<shm::MappingType::SINGLE>>(std::move(expected_memory.value()));
    _message_buffer = std::make_optional<shm::ring_buffer<reference_counted<T_Data>>>(_message_memory.value().addr(),
                                                                                      _message_memory.value().size());
    logging::info("Publisher<'{}'>::_m_initialize_message_buffer(): created message buffer", this->_id);
    return {};
  }

  [[nodiscard]] std::size_t _m_num_observers() const override { return _notifier->num_observers(); }

  void _m_notify_observers(std::size_t index) { _notifier->notify_observers(index); }

 private:
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _message_memory;
  std::optional<shm::ring_buffer<reference_counted<T_Data>>> _message_buffer;
  std::unique_ptr<notifier_type> _notifier = nullptr;
};

/**
 * Specialization for default shm publisher
 */
template <typename T_Data>
class Publisher<T_Data, internal::ShmDefault> final : internal::Publisher_I<T_Data> {
 public:
  typedef T_Data data_type;
  typedef event::shm_notification_memory_layout<reference_counted<T_Data>, event::shm_atomic_notification_header>
      message_buffer_type;
  typedef typename message_buffer_type::message_type data_access_type;

 public:
  using internal::Publisher_I<T_Data>::Publisher_I;

  std::error_code initialize() override {
    if (_initialized) {
      logging::warn("Publisher<'{}'>::setup(): called but already set up", this->_id);
      return {};
    }
    if (const std::error_code error = _m_initialize_message_buffer()) {
      logging::error("Publisher<'{}'>::setup(): failed to setup message buffer: {}::{}", this->_id, error.category().name(),
                     error.message(), error.message());
      return error;
    }
    _initialized = true;
    logging::debug("Publisher<'{}'>::setup(): setup done", this->_id);
    return {};
  }

  void publish(data_access_type* data) {
    assert(_initialized);
    switch (_options.publish_policy) {
      case publisher::PublishPolicy::ALWAYS: {
        const std::size_t msg_id = data->message_number;
        _m_notify_observers(msg_id);
        logging::info("Publisher<'{}'>::publish(): published message id {}", this->_id, msg_id);
        break;
      }
      case publisher::PublishPolicy::SUBSCRIBED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::SUBSCRIBED", this->_id);
        break;
      }
      case publisher::PublishPolicy::TIMED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::TIMED", this->_id);
        break;
      }
    }
  }

  template <typename... T_Args>
  data_access_type* construct_and_get(T_Args&&... args) {
    assert(_initialized);
    logging::info("Publisher<'{}'>::construct_and_get(): constructed next message", this->_id);
    std::size_t msg_id = _message_buffer->header->message_counter.load(std::memory_order_acquire) + 1;
    return _message_buffer->message_buffer.emplace_at(msg_id, std::forward<T_Args>(args)...,
                                                      _m_num_observers() + (this->_options.history_capacity > 0));
  }

  template <typename... T_Args>
  void publish(T_Args&&... args) {
    assert(_initialized);
    switch (_options.publish_policy) {
      case publisher::PublishPolicy::ALWAYS: {
        auto msg_id = _message_buffer->header->message_counter.load(std::memory_order_acquire) + 1;
        _message_buffer->message_buffer.emplace_at(msg_id, std::forward<T_Args>(args)...,
                                                   _m_num_observers() + (this->_options.history_capacity > 0));
        _m_notify_observers(msg_id);
        logging::info("Publisher<'{}'>::publish(): published message at {}", this->_id, msg_id);
        break;
      }
      case publisher::PublishPolicy::SUBSCRIBED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::SUBSCRIBED", this->_id);
        break;
      }
      case publisher::PublishPolicy::TIMED: {
        logging::critical("Publisher<'{}'>::publish(): Not implemented for PublishPolicy::TIMED", this->_id);
        break;
      }
    }
  }

 private:
  std::error_code _m_initialize_message_buffer() {
    const std::size_t num_bytes = message_buffer_type::required_bytes_for(_options.queue_capacity);
    auto expected_memory =
        shm::MappedMemory<shm::MappingType::SINGLE>::open_or_create(utils::path_from_shm_id(this->_id + ".ipcpp.mrb.shm"), num_bytes);
    if (!expected_memory.has_value()) {
      // TODO: return error
      return {1, std::system_category()};
    }
    _message_memory =
        std::make_optional<shm::MappedMemory<shm::MappingType::SINGLE>>(std::move(expected_memory.value()));
    _message_buffer =
        std::make_optional<message_buffer_type>(_message_memory.value().addr(), _message_memory.value().size());
    logging::info("Publisher<'{}'>::_m_initialize_message_buffer(): created message buffer", this->_id);
    return {};
  }

  [[nodiscard]] std::size_t _m_num_observers() const override {
    assert(_initialized);
    return _message_buffer->header->num_subscribers.load(std::memory_order_acquire);
  }

  void _m_notify_observers(std::size_t index) {
    assert(_initialized);
    _message_buffer->header->message_counter.store(index, std::memory_order_release);
    logging::info("Publisher<'{}'>::_m_notify_observers(): message_id: {}", this->_id, index);
  }

 private:
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _message_memory;
  std::optional<message_buffer_type> _message_buffer;
  publisher::Options _options{};
  bool _initialized = false;
};

}  // namespace ipcpp::publish_subscribe