/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/types.h>
#include <ipcpp/utils/mutex.h>
#include <ipcpp/utils/logging.h>

namespace ipcpp::ps {

template <typename T_p>
class Message {
 public:
  /**
   * Access to underlying data: read/write locked depending on T_AM
   * @tparam T_AM
   */
  template <AccessMode T_AM>
  class Access {
   public:
    using pointer_type = std::conditional_t<(T_AM == AccessMode::WRITE), T_p* const, const T_p* const>;
    using reference_type = std::conditional_t<(T_AM == AccessMode::WRITE), T_p&, const T_p&>;
   public:
    explicit Access(Message<T_p>* message) : _message(message) {}
    ~Access() {
      if (_message == nullptr) { return; }
      if constexpr (T_AM == AccessMode::WRITE) {
        logging::debug("Message::Access<WRITE>: releasing write lock");
        _message->_mutex.unlock();
      } else /* AccessMode::READ */ {
        if (auto rr = _message->_remaining_references.fetch_sub(1); rr == 1) {
          // this is actually a write operation
          _message->_opt_value.reset();
          logging::debug("Message::Access<READ>: destructing message");
        } else {
          logging::debug("Message::Access<READ>: remaining references: {}", rr - 1);
        }
        logging::debug("Message::Access<READ>: releasing read lock");
        _message->_mutex.unlock_shared();
        _message->_active_reference_counter.fetch_sub(1);
      }
    }

    Access(const Access&) = delete;
    Access& operator=(const Access&) = delete;

    Access(Access&& other) noexcept {
      std::swap(other._message, _message);
    }

    Access& operator=(Access&&) = delete;

    void reset() requires (T_AM == AccessMode::WRITE) {
      _message->_opt_value.reset();
    }

   template <typename... T_Args>
   void emplace(const std::int64_t remaining_references, std::uint64_t message_id, T_Args&&... args) requires (T_AM == AccessMode::WRITE){
     _message->_remaining_references.store(remaining_references);
     _message->_initial_references = remaining_references;
     _message->_opt_value.emplace(std::forward<T_Args>(args)...);
     _message->_message_id = message_id;
     logging::debug("Message::Access::emplace({}, {}, ...)", remaining_references, message_id);
   }

   pointer_type operator->() {
     return &(_message->_opt_value.value());
   }

   reference_type operator*() {
     return _message->_opt_value.value();
   }

   private:
    Message<T_p>* _message = nullptr;
  };

 template <AccessMode>
 friend class Access;

 public:
  Message() = default;
  template <typename... T_Args>
  explicit(sizeof...(T_Args) == 0) Message(std::int64_t remaining_references, std::uint64_t message_id, T_Args&&... args)
      : _opt_value(T_p(std::forward<T_Args>(args)...)),
        _message_id(message_id),
        _remaining_references(remaining_references),
        _initial_references(remaining_references) {}

  ~Message() = default;

  std::optional<Access<AccessMode::READ>> consume() {
    if (!_opt_value) {
      // no value available
      return std::nullopt;
    }
    if (_active_reference_counter.fetch_add(1) >= _remaining_references) {
      // access not approved: more accesses than initial references can result in UB because the value may be reset
      //  while accessing it
      return std::nullopt;
    }
    // (_initial_references * 2) is the amount of possible false negative try_locks because for each reference, two
    //  changes to the mutexes shared access flag can occur (one for acquiring and one for releasing).
    if (!_mutex.try_lock_shared((_initial_references * 2) + 1)) {
      // most likely WRITE acquired
      return std::nullopt;
    }
    return Access<AccessMode::READ>(this);
  }

  std::optional<Access<AccessMode::WRITE>> request_writable() {
    if (_mutex.try_lock()) {
      return Access<AccessMode::WRITE>(this);
    }
    // value is currently accessed
    return std::nullopt;
  }

  [[nodiscard]] std::uint64_t message_id() const { return _message_id; }
  [[nodiscard]] std::int64_t remaining_references() const { return _remaining_references.load(std::memory_order_acquire); }

 private:
  shared_mutex _mutex;
  std::optional<T_p> _opt_value;
  std::uint64_t _message_id = std::numeric_limits<std::uint64_t>::max();
  std::atomic_int64_t _remaining_references = 0;
  std::int64_t _initial_references = 0;
  std::atomic_int64_t _active_reference_counter = 0;
};

}  // namespace ipcpp::ps