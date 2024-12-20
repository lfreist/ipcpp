/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/event/observer.h>
#include <ipcpp/publish_subscribe/error.h>

#include <string>

namespace ipcpp::publish_subscribe {

namespace internal {

template <typename...>
struct ShmDefault {};

template <typename T_Data>
class Subscriber_I {
  public:
   explicit Subscriber_I(std::string&& identifier) : _id(std::move(identifier)) {}
   Subscriber_I(std::string&& identifier, const subscriber::Options& options) : _id(std::move(identifier)), _options(options) {}
   virtual ~Subscriber_I() = default;

   Subscriber_I& set_options(const subscriber::Options& options) {
     if (_initialized) {
       logging::warn("Subscriber<'{}'>::set_options(): called but Publisher already set up", _id);
     } else {
       _options = options;
     }
     return *this;
   }

   virtual std::error_code initialize() = 0;
   virtual std::error_code subscribe() = 0;
   virtual std::error_code cancel_subscription() = 0;

   protected:
    std::string _id;
    subscriber::Options _options{};
    bool _initialized = false;
};

}

template <typename T_Data, template <typename...> typename T_Observer = internal::ShmDefault>
requires(event::concepts::is_observer<T_Observer<std::size_t>> ||
           std::is_same_v<T_Observer<std::size_t>, internal::ShmDefault<std::size_t>>)
class Subscriber final : public internal::Subscriber_I<T_Data> {
public:
  typedef T_Data data_type;
  typedef reference_counted<T_Data> data_access_type;
  typedef T_Observer<std::size_t> observer_type;

  public:
   using internal::Subscriber_I<T_Data>::Subscriber_I;

  std::error_code initialize() override {
    assert(!this->_initialized);
    if (this->_initialized) {
      logging::warn("Subscriber<'{}'>::initialize(): called but already set up", this->_id);
      return {};
    }
    if (const std::error_code error = _m_initialize_message_buffer()) {
      logging::error("Subscriber<'{}'>::initialize(): failed to initialize message buffer: {}::{}", this->_id,
                     error.category().name(), error.message(), error.message());
      return error;
    }
    if (const std::error_code error = _m_initialize_observer()) {
      logging::error("Subscriber<'{}'>::initialize(): failed to initialize observer: {}::{}", this->_id, error.category().name(),
                     error.message(), error.message());
      return error;
    }
    this->_initialized = true;
    logging::debug("Subscriber<'{}'>::initialize(): initialization done", this->_id);
    return {};
  }

  std::error_code receive(std::function<std::error_code(data_access_type&)> callback, const std::chrono::milliseconds timeout) {
    assert(this->_initialized);
    auto expected_notification = _observer->receive(timeout, [](std::size_t n) { return n; });
    if (!expected_notification.has_value()) {
      logging::warn("Subscriber<'{}'>::receive(): observer received error", this->_id);
      // TODO: return error
      return {};
    }
    std::size_t index = expected_notification.value();
    data_access_type& data = _message_buffer->operator[](index);
    return callback(data);
  }

  std::error_code subscribe() override {
    assert(this->_initialized);
    return _observer->subscribe();
  }

  std::error_code cancel_subscription() override {
    assert(this->_initialized);
    return _observer->cancel_subscription();
  }

private:
  std::error_code _m_initialize_message_buffer() {
    auto expected_memory =
        shm::MappedMemory<shm::MappingType::SINGLE>::open("/" + this->_id + ".ipcpp.mrb.shm", AccessMode::WRITE);
    if (!expected_memory.has_value()) {
      // TODO: return error
      return {};
    }
    _message_memory =
        std::make_optional<shm::MappedMemory<shm::MappingType::SINGLE>>(std::move(expected_memory.value()));
    _message_buffer = std::make_optional<shm::ring_buffer<reference_counted<T_Data>>>(_message_memory.value().addr());
    logging::info("Subscriber<'{}'>::_m_initialize_message_buffer(): opened message buffer", this->_id);
    return {};
  }
  std::error_code _m_initialize_observer() {
    auto expected_observer = observer_type::create(std::string(this->_id));
    if (!expected_observer.has_value()) {
      // TODO: return error
      return {};
    }
    _observer = std::make_unique<observer_type>(std::move(expected_observer.value()));
    logging::info("Subscriber<'{}'>::_m_initialize_observer(): created observer", this->_id);
    return {};
  }

private:
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _message_memory;
  std::optional<shm::ring_buffer<reference_counted<T_Data>>> _message_buffer;
  std::unique_ptr<observer_type> _observer = nullptr;
};

template <typename T_Data>
class Subscriber<T_Data, internal::ShmDefault> final : public internal::Subscriber_I<T_Data> {
public:
  typedef T_Data data_type;
  typedef reference_counted<T_Data> data_access_type;
  typedef event::shm_notification_memory_layout<reference_counted<T_Data>, event::shm_atomic_notification_header>
      message_buffer_type;

  public:
   using internal::Subscriber_I<T_Data>::Subscriber_I;

  std::error_code initialize() override {
    assert(!this->_initialized);
    if (this->_initialized) {
      logging::warn("Subscriber<'{}'>::initialize(): called but already set up", this->_id);
      return {};
    }
    if (const std::error_code error = _m_initialize_message_buffer()) {
      logging::error("Subscriber<'{}'>::initialize(): failed to initialize message buffer: {}::{}", this->_id,
                     error.category().name(), error.message(), error.message());
      return error;
    }
    this->_initialized = true;
    logging::debug("Subscriber<'{}'>::initialize(): initialization done", this->_id);
    return {};
  }

  std::error_code receive(std::function<std::error_code(data_access_type&)> callback, const std::chrono::milliseconds timeout) {
    assert(this->_initialized);
    volatile std::int64_t received_message_number = 0;
    while (true) {
      received_message_number = _message_buffer->header->message_counter.load(std::memory_order_relaxed);
      if (received_message_number != _last_message_id) {
        break;
      }
      std::this_thread::yield();
    }
    if (received_message_number == -1) {
      logging::warn("Subscriber<'{}'>::receive(): Publisher down", this->_id);
      return {0, std::system_category()};
    }
    _last_message_id++;
    auto* notification = &_message_buffer->message_buffer[_last_message_id];
    if (notification->message_number != _last_message_id) {
      logging::error("Subscriber<'{}'>::receive(): Message invalid: message number mismatch (received: {}, actual: {})", this->_id, _last_message_id, notification->message_number);
      return {1, std::system_category()};
    }
    logging::debug("Subscriber<'{}'>::receive(): received message id {}", this->_id, notification->message_number);
    auto& data = notification->message;
    return callback(data);
  }

  std::error_code subscribe() override {
    assert(this->_initialized);
    if (!this->_initialized) {
      return {1, std::system_category()};
    }
    _message_buffer->header->num_subscribers.fetch_add(1, std::memory_order_release);
    _last_message_id = _message_buffer->header->message_counter.load(std::memory_order_acquire);
    return {};
  }

  std::error_code cancel_subscription() override {
    assert(this->_initialized);
    _message_buffer->header->num_subscribers.fetch_sub(1, std::memory_order_release);
    // TODO: decrease reference counters for messages not yet read
    return {};
  }

private:
  std::error_code _m_initialize_message_buffer() {
    auto expected_memory =
        shm::MappedMemory<shm::MappingType::SINGLE>::open(utils::path_from_shm_id(this->_id + ".ipcpp.mrb.shm"), AccessMode::WRITE);
    if (!expected_memory.has_value()) {
      // TODO: return error
      return {1, std::system_category()};
    }
    _message_memory =
        std::make_optional<shm::MappedMemory<shm::MappingType::SINGLE>>(std::move(expected_memory.value()));
    _message_buffer = std::make_optional<message_buffer_type>(_message_memory.value().addr());
    logging::info("Subscriber<'{}'>::_m_initialize_message_buffer(): opened message buffer", this->_id);
    return {};
  }

private:
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _message_memory;
  std::optional<message_buffer_type> _message_buffer;
  std::int64_t _last_message_id = -1;
};

}