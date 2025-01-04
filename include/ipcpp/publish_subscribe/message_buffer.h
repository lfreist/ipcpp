/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/synced.h>

#include <atomic>
#include <expected>
#include <new>
#include <span>
#include <system_error>

namespace ipcpp::ps {

template <typename T_p>
  requires(!std::is_reference_v<T_p> && !std::is_pointer_v<T_p>)
class message_buffer {
 public:
  typedef T_p value_type;

 public:
  struct Header {
    // aligned to be placed on its own cache line
    alignas(std::hardware_destructive_interference_size) std::atomic_uint_fast32_t num_subscribers = 0;

    // aligned to be placed together on a single cache line: final is only be read/written if next changes 
    struct {
      // TODO: what happens if actually 0xFFFF messages were sent?
      //   Possible Solution: If publisher goes down, next is set to 0xFFFF and subscribers receiving 0xFFFF first check
      //                      if final is 0 or not.
      //   IMPORTANT: final must be set before next is changed then!

      /// this is used internally to lock free access a T_p without notifying to subscribers
      alignas(std::hardware_destructive_interference_size) std::atomic_uint_fast32_t next = 0;
      // TODO: can we avoid that publishers need to access and modify next and published?
      //       It would be better to handle all this using a single atomic value (performance-wise)...

      /// this is used to actually notify subscribers about a new T_p
      alignas(std::hardware_destructive_interference_size) std::atomic_uint_fast32_t published = std::numeric_limits<std::uint_fast32_t>::max();
      alignas(std::hardware_destructive_interference_size) std::atomic_uint_fast32_t final = 0;
    } message_id;

    // general buffer information
    struct alignas(std::hardware_destructive_interference_size) {
      /// capacity must be a power of two to allow efficient message_id to index transformations
      std::uint_fast32_t capacity = 0;
      std::uint_fast32_t history_size = 0;
      std::atomic_bool initialized = false;
    } buffer_info;
  };

 public:
  static std::size_t required_size_bytes(const std::size_t capacity) {
    return sizeof(Header) + (sizeof(T_p) * numeric::floor_to_power_of_two(capacity));
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<message_buffer<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                        T_Args&&... args) {
    logging::debug("message_buffer::init_at()");
    if (size_bytes < (sizeof(Header) + sizeof(T_p))) {
      // TODO: add proper errors
      logging::warn("message_buffer::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    Header* header = std::construct_at(reinterpret_cast<Header*>(addr));

    std::uint64_t capacity = numeric::floor_to_power_of_two((size_bytes - sizeof(Header)) / sizeof(T_p));
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(Header)), capacity);
    for (auto& elem : buffer) {
      std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
    }

    header->buffer_info.capacity = capacity > std::numeric_limits<uint_fast32_t>::max() ? std::numeric_limits<uint_fast32_t>::max() : static_cast<std::uint_fast32_t>(capacity);
    // TODO: handle history size here!
    header->buffer_info.initialized.store(true, std::memory_order_release);
    return message_buffer(header, buffer);
  }

  static std::expected<message_buffer, std::error_code> read_at(std::uintptr_t addr) {
    logging::debug("message_buffer::read_at()");

    auto* header = reinterpret_cast<Header*>(addr);

    if (!header->buffer_info.initialized.load(std::memory_order_acquire)) {
      logging::warn("message_buffer::read_at: memory not initialized (using message_buffer::init_at)");
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(Header)), header->buffer_info.capacity);

    return message_buffer(header, buffer);
  }

 public:
  value_type& operator[](std::uint_fast32_t message_id) {
    std::uint_fast32_t index = message_id & (_buffer.size() - 1);
    logging::debug("message_buffer::operator[]({}) -> {}", message_id, index);
    return _buffer[index];
  }

  const value_type& operator[](std::uint_fast32_t message_id) const {
    std::uint_fast32_t index = message_id & (_buffer.size() - 1);
    logging::debug("message_buffer::operator[]({}) -> {}", message_id, index);
    return _buffer[index];
  }

  [[nodiscard]] std::uint_fast32_t size() const { return static_cast<std::uint_fast32_t>(_buffer.size()); }

  Header* header() { return _header; }

 private:
  message_buffer(Header* header, std::span<value_type> queue_items) : _header(header), _buffer(queue_items) {}

 private:
  Header* _header;
  std::span<value_type> _buffer;
};

}  // namespace ipcpp::ps