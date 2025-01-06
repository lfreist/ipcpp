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

namespace ipcpp::ps::rt {

template <typename T_p>
class Message {
 public:
  static constexpr uint_t invalid_id_v = std::numeric_limits<uint_t>::max();

 public:
  class Access {
   public:
    Access() = default;
    explicit Access(Message<T_p>* message) : _message(message) {}
    ~Access() {
      if (_message == nullptr) {
        return;
      }
      // TODO: does memory order matter here?
      if (_message->_active_reference_counter.fetch_sub(1, std::memory_order_release) == 1) {
        logging::debug("Message::Access<READ>: destructing message");
        reset();
      }
      logging::debug("Message::Access: releasing access");
    }

    Access(const Access&) = delete;
    Access& operator=(const Access&) = delete;

    Access(Access&& other) noexcept { std::swap(other._message, _message); }

    Access& operator=(Access&& other) noexcept {
      std::swap(other._message, _message);
      return *this;
    }

    void reset() {
      logging::debug("rt::Message::Access::reset(): index: {}", _message->id());
      _message->_opt_value.reset();
      _message->_message_id = Message<T_p>::invalid_id_v;
    }

    T_p* operator->() { return &(_message->_opt_value.value()); }
    const T_p* operator->() const { return &(_message->_opt_value.value()); }

    T_p& operator*() { return _message->_opt_value.value(); }
    const T_p& operator*() const { return _message->_opt_value.value(); }

    operator bool() const { return _message->_opt_value.has_value(); }

   private:
    Message<T_p>* _message = nullptr;
  };

  friend class Access;

 public:
  Message() = default;
  ~Message() = default;

  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;
  Message(Message&&) = delete;
  Message& operator=(Message&&) = delete;

  [[nodiscard]] uint_t active_references() const { return _active_reference_counter.load(std::memory_order_relaxed); }

  template <typename... T_Args>
  void emplace(uint_t message_id, T_Args&&... args) {
    _opt_value.emplace(std::forward<T_Args>(args)...);
    _message_id = message_id;
    logging::debug("Message::Access::emplace({}, ({})...)", message_id, sizeof...(T_Args));
  }

  std::optional<Access> acquire() {
    uint_t active_references = _active_reference_counter.fetch_add(1);
    if (active_references < 1 || !_opt_value) {
      // no value available
      logging::warn("Message::acquire: no value available");
      _active_reference_counter.fetch_sub(1);
      return std::nullopt;
    }
    return Access(this);
  }

  /**
   * Construct an Access without checking if `this` is a valid message!
   *  Internally used by publishers that just defines `this` to become a valid message
   *
   * @warning only call if you are ABSOLUTELY sure that `this` is a valid message.
   *
   * @return
   */
  Access acquire_unsafe() {
    _active_reference_counter.fetch_add(1, std::memory_order_release);
    return Access(this);
  }

  [[nodiscard]] uint_t id() const { return _message_id; }

 private:
  std::optional<T_p> _opt_value = std::nullopt;
  alignas(std::hardware_destructive_interference_size) uint_t _message_id = invalid_id_v;
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> _active_reference_counter = 0;
};

}  // namespace ipcpp::ps::rt
