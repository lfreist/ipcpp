/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/synced.h>
#include <ipcpp/utils/logging.h>

#include <atomic>
#include <expected>
#include <new>
#include <span>
#include <system_error>

namespace ipcpp::ps {

template <typename T_p>
  requires(!std::is_reference_v<T_p> && !std::is_pointer_v<T_p>)
class shm_message_queue {
 public:
  typedef T_p value_type;

 public:
  struct Header {
    // aligned to be placed by its own on a single cache line
    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t num_subscribers = 0;

    // aligned to be placed on one cache line together because they do not change independently. last is only be read
    //  if next was set to 0xFFFFFFFF
    struct alignas(std::hardware_destructive_interference_size) {
      std::atomic_uint64_t next = 0;
      std::atomic_uint64_t last = 0;
    } message_id;

    /// size of the queue. Also indicates successfully initialization of the queue
    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t queue_size = 0;
    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t history_size = 0;

    // in memory, here go the actual queue data if memory_layout is allocated at the beginning of the provided memory
  };

  static_assert(sizeof(Header) == 256);

 public:
  static std::size_t required_size_bytes(const std::size_t queue_size) {
    return sizeof(Header) + (sizeof(T_p) * queue_size);
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<shm_message_queue<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                        T_Args&&... args) {
    logging::debug("shm_message_queue::init_at()");
    if (size_bytes < (sizeof(Header) + sizeof(T_p))) {
      // TODO: add proper errors
      logging::warn("shm_message_queue::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    Header* header = std::construct_at(reinterpret_cast<Header*>(addr));

    std::uint64_t queue_size = numeric::floor_to_power_of_two((size_bytes - sizeof(Header)) / sizeof(T_p));
    std::span<T_p> queue(reinterpret_cast<T_p*>(addr + sizeof(Header)), queue_size);
    for (auto& elem : queue) {
      std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
    }

    header->queue_size.store(queue_size, std::memory_order_release);  // TODO: first call initialize!
    return shm_message_queue(header, queue);
  }

  static std::expected<shm_message_queue, std::error_code> read_at(std::uintptr_t addr) {
    logging::debug("shm_message_queue::read_at()");
    auto* header = reinterpret_cast<Header*>(addr);
    if (header->queue_size.load(std::memory_order_acquire) == 0) {
      logging::warn("shm_message_queue::read_at: memory not initialized (using init_at)");
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    std::span<T_p> queue(reinterpret_cast<T_p*>(addr + sizeof(Header)), header->queue_size.load());

    return shm_message_queue(header, queue);
  }

 public:
  value_type& operator[](std::size_t message_id) {
    //std::size_t index = message_id % size();
    std::size_t index = message_id & (_queue_items.size() - 1);
    logging::debug("shm_message_queue::operator[]({}) -> {}", message_id, index);
    return _queue_items[index];
  }

  const value_type& operator[](std::size_t message_id) const {
    std::size_t index = message_id & (_queue_items.size() - 1);
    logging::debug("shm_message_queue::operator[]({}) -> {}", message_id, index);
    return _queue_items[index];
  }

  [[nodiscard]] std::size_t size() const { return _queue_items.size(); }

  Header* header() { return _header; }

 private:
  shm_message_queue(Header* header, std::span<value_type> queue_items)
      : _header(header), _queue_items(queue_items) {}

 private:
  Header* _header;
  std::span<value_type> _queue_items;
};

}  // namespace ipcpp::ps