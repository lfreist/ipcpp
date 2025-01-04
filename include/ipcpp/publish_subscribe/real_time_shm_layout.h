/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/publish_subscribe/real_time_message.h>
#include <ipcpp/utils/numeric.h>

#include <expected>
#include <span>
#include <string>

namespace ipcpp::ps {

/**
 * Memory Layout: | header | available index list | chunk list |
 * @tparam T_p
 */
template <typename T_p>
class RealTimeMemLayout {
 private:
  struct Header {
    // the first 32 bit are index, last 32 bit are message id
    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t message_info =
        std::numeric_limits<std::uint64_t>::max();

    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t num_subscribers = 0;

    alignas(std::hardware_destructive_interference_size) std::atomic_uint32_t head_idx = 0;
    alignas(std::hardware_destructive_interference_size) std::atomic_uint64_t capacity = 0;

    std::atomic_bool initialized = false;
  };

 public:
  static std::size_t required_size_bytes(const std::size_t queue_size) {
    return sizeof(Header) + (queue_size * sizeof(std::uint32_t)) + (queue_size * sizeof(T_p));
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<RealTimeMemLayout<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                        T_Args&&... args) {
    std::uint64_t capacity = (size_bytes - sizeof(Header)) / (sizeof(std::uint32_t) + sizeof(T_p));

    Header* header = std::construct_at(reinterpret_cast<Header*>(addr));
    header->capacity.store(capacity);

    std::span<std::uint32_t> index_list(reinterpret_cast<std::uint32_t*>(addr + sizeof(Header)), capacity);
    for (std::uint32_t i = 0; i < capacity; ++i) {
      index_list[i] = i;
    }
    header->head_idx = capacity - 1;

    std::span<T_p> chunk_list(reinterpret_cast<T_p*>(addr + sizeof(Header) + (capacity * sizeof(std::uint32_t))),
                              capacity);
    for (auto& chunk : chunk_list) {
      std::construct_at(std::addressof(chunk), std::forward<T_Args>(args)...);
    }

    header->initialized = true;

    RealTimeMemLayout self(header, index_list, chunk_list);
    return self;
  }

  static std::expected<RealTimeMemLayout<T_p>, std::error_code> read_at(std::uintptr_t addr) {
    auto* header = reinterpret_cast<Header*>(addr);
    if (!header->initialized) {
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    std::span<std::uint32_t> index_list(reinterpret_cast<std::uint32_t*>(addr + sizeof(Header)), header->capacity);
    std::span<T_p> chunk_list(
        reinterpret_cast<T_p*>(addr + sizeof(Header) + (header->capacity * sizeof(std::uint32_t))), header->capacity);

    RealTimeMemLayout self(header, index_list, chunk_list);

    return self;
  }

 public:
  T_p& operator[](std::uint32_t index) { return _chunk_list[index]; }
  const T_p& operator[](std::uint32_t index) const { return _chunk_list[index]; }

  std::uint32_t allocate() {
    std::uint32_t index = _header->head_idx.fetch_sub(1, std::memory_order_release);
    if (index == 0) {
      return std::numeric_limits<std::uint32_t>::max();
    }
    return _index_list[index];
  }

  void deallocate(std::uint32_t index) {
    if (index == std::numeric_limits<std::uint32_t>::max()) {
      return;
    }
    while (true) {
      std::uint32_t head_index = _header->head_idx.load(std::memory_order_acquire);
      std::uint32_t next_head = head_index + 1;
      _index_list[next_head] = index;
      if (_header->head_idx.compare_exchange_weak(head_index, next_head, std::memory_order_release)) {
        break;
      }
    }
  }

  Header* header() { return _header; }

 private:
  RealTimeMemLayout(Header* header, std::span<std::uint32_t> index_list, std::span<T_p> chunk_list)
      : _header(header), _index_list(index_list), _chunk_list(chunk_list) {}

 private:
  Header* _header = nullptr;
  std::span<std::uint32_t> _index_list;
  std::span<T_p> _chunk_list;
};

}  // namespace ipcpp::ps