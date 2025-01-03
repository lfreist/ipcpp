/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/types.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/mutex.h>

#include <atomic>
#include <numeric>
#include <optional>

namespace ipcpp::ps {

template <typename T_p>
class RealTimeMemLayout;

namespace rt {

template <typename T_p>
class Message {
 public:
  typedef std::uint64_t msg_id_type;
  static constexpr msg_id_type invalid_id_v = std::numeric_limits<msg_id_type>::max();

 public:
  template <AccessMode T_AM>
  class Access {
   public:
    using pointer_type = std::conditional_t<(T_AM == AccessMode::WRITE), T_p* const, const T_p* const>;
    using reference_type = std::conditional_t<(T_AM == AccessMode::WRITE), T_p&, const T_p&>;

   public:
    Access(Message<T_p>* message, RealTimeMemLayout<Message<T_p>>& mem_layout) : _message(message), _mem_layout(mem_layout) {}
    ~Access() {
      if (_message == nullptr) {
        return;
      }
      if constexpr (T_AM == AccessMode::READ) {
        if (_message->_active_reference_counter.fetch_sub(1, std::memory_order_release) == 1) {
          // this is intended to perform a write operation!
          reset();
          logging::debug("Message::Access<READ>: destructing message");
        }
        logging::debug("Message::Access<READ>: releasing read lock");
      }
    }

    Access(const Access&) = delete;
    Access& operator=(const Access&) = delete;

    Access(Access&& other) noexcept : _mem_layout(other._mem_layout) { std::swap(other._message, _message); }

    Access& operator=(Access&& other) noexcept {
      _mem_layout = other._mem_layout;
      std::swap(other._message, _message);
      return *this;
    }

    void reset() {
      logging::debug("rt::Message::Access::reset(): message_id: {}, index: {}", _message->_msg_id.load(), _message->_index);
      _message->_opt_value.reset();
      _message->_msg_id = Message<T_p>::invalid_id_v;
      _mem_layout.deallocate(_message->_index);
      _message->_index = Message<T_p>::invalid_id_v;
    }

    template <typename... T_Args>
    void emplace(std::uint64_t index, std::uint64_t message_id, T_Args&&... args)
      requires(T_AM == AccessMode::WRITE)
    {
      _message->_opt_value.emplace(std::forward<T_Args>(args)...);
      _message->_msg_id = message_id;
      _message->_index = index;
      logging::debug("Message::Access::emplace({}, ({})...)", message_id, sizeof...(T_Args));
    }

    pointer_type operator->() { return &(_message->_opt_value.value()); }

    reference_type operator*() { return _message->_opt_value.value(); }

    [[nodiscard]] std::uint64_t message_id() const { return _message->_msg_id; }

   private:
    Message<T_p>* _message = nullptr;
    RealTimeMemLayout<Message<T_p>>& _mem_layout;
  };

  template <AccessMode>
  friend class Access;

 public:
  Message() = default;
  ~Message() = default;

  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;
  Message(Message&&) = delete;
  Message& operator=(Message&&) = delete;

  [[nodiscard]] std::uint64_t active_references() const {
    return _active_reference_counter.load(std::memory_order_acquire);
  }

  std::optional<Access<AccessMode::READ>> acquire(RealTimeMemLayout<Message<T_p>>& mem_layout) {
    if (!_opt_value) {
      // no value available
      logging::warn("Message::acquire: no value available");
      return std::nullopt;
    }
    _active_reference_counter.fetch_add(1);
    if (_msg_id.load(std::memory_order_acquire) == std::numeric_limits<std::uint64_t>::max()) {
      // most likely WRITE acquired
      logging::warn("Message::acquire: read access failed");
      _active_reference_counter.fetch_sub(1);
      return std::nullopt;
    }
    return Access<AccessMode::READ>(this, mem_layout);
  }

  Access<AccessMode::WRITE> request_writable(RealTimeMemLayout<Message<T_p>>& mem_layout) {
    return Access<AccessMode::WRITE>(this, mem_layout);
  }

  void set_message_id(std::uint64_t id) { _msg_id.store(id, std::memory_order_release); }

  [[nodiscard]] std::uint64_t index() const { return _index; }
  [[nodiscard]] std::uint64_t id() const { return _msg_id.load(std::memory_order_acquire); }

 private:
  std::optional<T_p> _opt_value = std::nullopt;

  std::atomic_uint64_t _active_reference_counter = 0;
  std::atomic_uint64_t _msg_id = invalid_id_v;
  std::uint64_t _index = invalid_id_v;
};

}  // namespace rt
}  // namespace ipcpp::ps