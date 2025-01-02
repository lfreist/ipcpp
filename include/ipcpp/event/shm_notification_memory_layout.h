/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/utils/utils.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace ipcpp::event {

struct shm_atomic_notification_header {
  alignas(std::hardware_destructive_interference_size) std::atomic_int64_t message_counter = -1;
  alignas(std::hardware_destructive_interference_size) std::atomic_size_t num_subscribers = 0;
};

struct shm_condition_variable_notification_header {
  std::int64_t message_counter = -1;
  std::size_t latest_buffer_index = 0;
  std::size_t num_subscribers = 0;
  std::mutex mutex;
  std::condition_variable cv;
};

template <typename T_Message, typename T_Header>
struct shm_notification_memory_layout {
  static std::size_t required_bytes_for(std::size_t queue_size) {
    return utils::align_up(sizeof(T_Header)) + shm::ring_buffer<message_type>::required_bytes_for(queue_size);
  }

  /// a message is a tuple consisting of the message and the message_number
  struct message_type {
    template <typename... T_Args>
    explicit message_type(const std::int64_t msg_number, T_Args&&... args)
        : message(std::forward<T_Args>(args)...), message_number(msg_number) {}
    message_type() = default;
    T_Message message{};
    std::int64_t message_number = -1;
  };
  typedef T_Header header_type;

  template <typename... T_HeaderArgs>
  shm_notification_memory_layout(std::uintptr_t addr, const std::size_t size, T_HeaderArgs&&... args)
      : header(std::construct_at(reinterpret_cast<header_type*>(addr), std::forward<T_HeaderArgs>(args)...)),
        message_buffer(addr + utils::align_up(sizeof(header_type)), size - utils::align_up(sizeof(header_type))) {}

  explicit shm_notification_memory_layout(std::uintptr_t addr)
      : header(reinterpret_cast<header_type*>(addr)), message_buffer(addr + utils::align_up(sizeof(header_type))) {}

  shm_notification_memory_layout(shm_notification_memory_layout&& other) noexcept : message_buffer(std::move(other.message_buffer)) {
    std::swap(header, other.header);
  }

  shm_notification_memory_layout& operator=(shm_notification_memory_layout&& other) noexcept {
    message_buffer = std::move(other.message_buffer);
    std::swap(header, other.header);
    return *this;
  }

  header_type* header = nullptr;
  shm::ring_buffer<message_type> message_buffer;
};

}  // namespace ipcpp::event