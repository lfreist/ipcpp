/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

/*
 * NOTES:
 * We can speed up multi publisher scenarios:
 * 1. add a T_p pool per possible publisher
 *    -> publishers can have their non-atomic own next counter
 * 2. the size of each T_p pool depends on the maximum number of subscribers:
 *    -> for x subscribers x + 2 T_p's per pool are good: x for published data accessed by a subscriber and 2 for
 * alternating publishing new data
 *
 * Possible memory layout:
 *  |--------------------------------------------------|
 *  | common header                                    |
 *  | list of (max publishers) per-publisher-headers   |
 *  | list of (num T_p pools + T_p pool size) T_p      |
 *  | -------------------------------------------------|
 *
 *  - common header:
 *    - atomic int: latest published index
 *    - int: max_num_subscribers -> defines the size of a single T_p pool
 *      -> should bne rounded to power of two to allow fast wrap-around on overflow
 *    - int: max_num_publishers -> defines the number of T_p pools
 *      -> (max_num_subscribers + 2) * _max_num_publishers is the total number of T_p
 *      -> publisher_id * (_max_num_subscribers + 2) is the first T_p that publisher_id can use
 */

#pragma once

#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/atomic.h>

#include <atomic>
#include <expected>
#include <new>
#include <span>
#include <system_error>

namespace ipcpp::ps::mpmc {

namespace internal {

template <typename T>
struct half_size_int;

template <>
struct half_size_int<uint64_t> {
  using type = uint32_t;
};

template <>
struct half_size_int<uint32_t> {
  using type = uint16_t;
};

template <>
struct half_size_int<uint16_t> {
  using type = uint8_t;
};

template <>
struct half_size_int<uint8_t> {
  using type = uint8_t;
};

}

static_assert(!std::is_same_v<atomic::largest_lock_free_uint, void>, "No lock-free type found!");

template <typename T_p>
class message_buffer {
 public:
  typedef std::remove_cvref_t<T_p> value_type;
  typedef atomic::largest_lock_free_uint uint_t;
  typedef std::atomic<atomic::largest_lock_free_uint> atomic_uint_t;

 public:
  struct CommonHeader {
    CommonHeader(std::uint_fast32_t max_subscribers_, std::uint_fast32_t max_publishers_,
                 std::uint_fast32_t history_size_ = 0)
        : max_subscribers(max_subscribers_), max_publishers(max_publishers_), history_size(history_size_) {}

    alignas(std::hardware_destructive_interference_size) atomic_uint_t num_subscribers = 0;
    alignas(std::hardware_destructive_interference_size) const std::uint_fast32_t max_subscribers = 0;

    alignas(std::hardware_destructive_interference_size) atomic_uint_t num_publishers = 0;
    alignas(std::hardware_destructive_interference_size) const std::uint_fast32_t max_publishers = 0;

    // TODO: implement history logic:
    //       Concept: reserve space for <history_size> indices and add a rotating head-index that points to the oldest
    //                index in history and is overwritten on the next publish
    alignas(std::hardware_destructive_interference_size) const std::uint_fast32_t history_size = 0;

    // this is the value that is actually polled by subscribers
    // first half is index, second half is local message id. the combination makes it unique since each publisher gets
    //  its index range
    alignas(std::hardware_destructive_interference_size) atomic_uint_t latest_published = 0;
    // if all publishers went down, final_published is set to latest_published and latest_published is set to 0xF...F
    alignas(std::hardware_destructive_interference_size) uint_t final_published = 0;

    alignas(std::hardware_destructive_interference_size) bool initialized = false;
  };
  struct PerPublisherHeader {
    // counter starting at 0
    alignas(std::hardware_destructive_interference_size) uint_t next_local_message_id = 0;
    alignas(std::hardware_destructive_interference_size) bool is_online = false;
  };

 public:
  static std::size_t per_publisher_pool_size(std::uint_fast32_t max_subscribers) {
    // rounded to power of two to allow fast wrap-around of index overflows. This is necessary because we track a local
    // message id from which we need to access a T_p in the publishers pool.
    return numeric::ceil_to_power_of_two(max_subscribers + 2);
  }

  static std::size_t required_size_bytes(std::uint_fast32_t max_subscribers, std::uint_fast32_t max_publishers) {
    return sizeof(CommonHeader)                                       // one common header
           + (sizeof(PerPublisherHeader) * max_publishers)            // each publisher needs a PerPublisherHeader
           + (sizeof(T_p) * per_publisher_pool_size(max_subscribers) * max_publishers);  // each publisher needs <max_subscribers + 2> T_ps
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<message_buffer<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                     std::uint_fast32_t max_subscribers,
                                                                     std::uint_fast32_t max_publishers,
                                                                     T_Args&&... args) {
    logging::debug("message_buffer::init_at()");
    if (size_bytes < required_size_bytes(max_subscribers, max_publishers)) {
      // TODO: add proper errors
      logging::warn("message_buffer::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    std::uint64_t capacity = (max_subscribers + 2) * max_publishers;
    if (capacity > (std::numeric_limits<uint_t>::max() / 2)) {
      // TODO: error is that it is impossible to store the index of all elements in haf a uint_t
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    CommonHeader* header = std::construct_at(reinterpret_cast<CommonHeader*>(addr), max_subscribers, max_publishers, 0);

    std::span<PerPublisherHeader> per_publisher_headers(reinterpret_cast<PerPublisherHeader*>(addr + sizeof(CommonHeader)), max_publishers);
    for (auto& pp_header : per_publisher_headers) {
      std::construct_at(std::addressof(pp_header));
    }

    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(CommonHeader) + (sizeof(PerPublisherHeader) * max_publishers)), capacity);
    for (auto& elem : buffer) {
      std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
    }

    // TODO: handle history size here!
    header->initialized.store(true, std::memory_order_release);
    return message_buffer(header, per_publisher_headers, buffer);
  }

  static std::expected<message_buffer, std::error_code> read_at(std::uintptr_t addr) {
    logging::debug("message_buffer::read_at()");

    auto* header = reinterpret_cast<CommonHeader*>(addr);

    std::uint64_t capacity = (header->max_subscribers + 2) * header->max_publishers;

    if (!header->initialized.load(std::memory_order_acquire)) {
      logging::warn("message_buffer::read_at: memory not initialized (using message_buffer::init_at)");
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    std::span<PerPublisherHeader> pp_headers(reinterpret_cast<PerPublisherHeader*>(addr + sizeof(CommonHeader)), header->max_publishers);
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(CommonHeader) + (sizeof(PerPublisherHeader) * header->max_publishers)), capacity);

    return message_buffer(header, pp_headers, buffer);
  }

 public:
  value_type& operator[](internal::half_size_int<uint_t>::type index) {
    return _buffer[index];
  }

  const value_type& operator[](internal::half_size_int<uint_t>::type index) const {
    return _buffer[index];
  }

  value_type& get(std::uint_fast32_t publisher_id, uint_t local_message_id) {
    return _buffer[publisher_id * (local_message_id & (_h_wrap_around_value))];
  }

  const value_type& get(std::uint_fast32_t publisher_id, uint_t local_message_id) const {
    return _buffer[publisher_id * (local_message_id & (_h_wrap_around_value))];
  }

  [[nodiscard]] std::size_t size() const { return _buffer.size(); }

  CommonHeader* common_header() { return _common_header; }
  PerPublisherHeader* per_publisher_header(std::uint_fast32_t publisher_id) { return _pp_headers[publisher_id]; }

 private:
  message_buffer(CommonHeader* header, std::span<PerPublisherHeader> pp_headers, std::span<value_type> queue_items) : _common_header(header), _pp_headers(pp_headers), _buffer(queue_items) {
    _h_wrap_around_value = per_publisher_pool_size(header->num_subscribers) - 1;
  }

 private:
  CommonHeader* _common_header;
  std::span<PerPublisherHeader> _pp_headers;
  std::span<value_type> _buffer;
  // internally used for wrap-around of local message id to index: per_publisher_pool_size(header->num_subscribers) - 1
  //  stored to avoid recomputations
  uint_t _h_wrap_around_value;
};

}  // namespace ipcpp::ps