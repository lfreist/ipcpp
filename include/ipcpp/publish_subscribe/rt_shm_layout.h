/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/types.h>
#include <ipcpp/utils/atomic.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/utils.h>

#include <atomic>
#include <chrono>
#include <expected>
#include <new>
#include <span>
#include <system_error>
#include <thread>

namespace ipcpp::ps {

using namespace std::chrono_literals;

struct RealTimePublisherEntry {
  RealTimePublisherEntry(uint_half_t id) : id(id), creation_timestamp(utils::timestamp()), pid(utils::get_pid()) {}
  RealTimePublisherEntry() = default;

  const uint_half_t id = 0;
  const std::uint64_t pid = 0;
  const std::int64_t creation_timestamp = -1;

  alignas(std::hardware_destructive_interference_size) uint_half_t next_local_message_id = 0;
  alignas(std::hardware_destructive_interference_size) bool is_online = false;

  [[nodiscard]] bool is_available() const { return id == 0 && pid == 0 && creation_timestamp == -1; }
};

struct RealTimeSubscriberEntry {
  RealTimeSubscriberEntry(uint_half_t id) : id(id), creation_timestamp(utils::timestamp()), pid(utils::get_pid()) {}
  RealTimeSubscriberEntry() = default;

  const uint_half_t id = 0;
  const std::uint64_t pid = 0;
  const std::int64_t creation_timestamp = -1;

  alignas(std::hardware_destructive_interference_size) bool is_online = false;

  [[nodiscard]] bool is_available() const { return id == 0 && pid == 0 && creation_timestamp == -1; }
};

struct RealTimeInstanceData {
  RealTimeInstanceData(uint_half_t max_publishers_, uint_half_t max_subscribers_, std::uint64_t data_shm_hash,
                       uint_half_t history_size_ = 0)
      : max_publishers(max_publishers_),
        max_subscribers(max_subscribers_),
        data_shm_hash(data_shm_hash),
        history_size(history_size_) {}

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> num_subscribers = 0;
  alignas(std::hardware_destructive_interference_size) const uint_half_t max_subscribers = 0;

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> num_publishers = 0;
  alignas(std::hardware_destructive_interference_size) const uint_half_t max_publishers = 0;

  // hash indicating the corresponding data_shm
  const std::uint64_t data_shm_hash;

  // TODO: implement history logic:
  //       Concept: reserve space for <history_size> indices and add a rotating head-index that points to the oldest
  //                index in history and is overwritten on the next publish
  alignas(std::hardware_destructive_interference_size) const uint_half_t history_size = 0;

  // this is the value that is actually polled by subscribers
  // first half is index, second half is local message id. the combination makes it unique since each publisher gets
  //  its index range
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> latest_published =
      std::numeric_limits<uint_t>::max();
  // if all publishers went down, final_published is set to latest_published and latest_published is set to 0xF...F
  alignas(std::hardware_destructive_interference_size) uint_t final_published = 0;

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> initialization_state = 0;

  // helper methods
  std::span<const RealTimePublisherEntry> get_publishers() const {
    return std::span(
        reinterpret_cast<const RealTimePublisherEntry*>(reinterpret_cast<const std::uint8_t*>(this) + sizeof(this)),
        max_subscribers);
  }

  std::span<const RealTimeSubscriberEntry> get_subscribers() const {
    return std::span(
        reinterpret_cast<const RealTimeSubscriberEntry*>(reinterpret_cast<const std::uint8_t*>(this) + sizeof(this) +
                                                         (max_subscribers * sizeof(RealTimeSubscriberEntry))),
        max_subscribers);
  }
  std::span<RealTimePublisherEntry> get_publishers() {
    return std::span(reinterpret_cast<RealTimePublisherEntry*>(reinterpret_cast<std::uint8_t*>(this) + sizeof(this)),
                     max_subscribers);
  }

  std::span<RealTimeSubscriberEntry> get_subscribers() {
    return std::span(reinterpret_cast<RealTimeSubscriberEntry*>(reinterpret_cast<std::uint8_t*>(this) + sizeof(this) +
                                                                (max_subscribers * sizeof(RealTimeSubscriberEntry))),
                     max_subscribers);
  }

  // static methods
  static std::size_t required_size(uint_half_t max_subscribers, uint_half_t max_publishers) {
    return sizeof(RealTimeInstanceData) + (sizeof(RealTimeSubscriberEntry) * max_subscribers) +
           (sizeof(RealTimePublisherEntry) * max_publishers);
  }
};

template <typename T_p>
class RealTimeMessageBuffer {
 public:
  typedef std::remove_cvref_t<T_p> value_type;

 public:
  static uint_half_t per_publisher_pool_size(uint_half_t max_subscribers) {
    assert(max_subscribers <= std::numeric_limits<uint_half_t>::max() - 2);
    // rounded to power of two to allow fast wrap-around of index overflows. This is necessary because we track a local
    // message id from which we need to access a T_p in the publishers pool.
    return numeric::ceil_to_power_of_two(max_subscribers + 2);
  }

  static uint_t required_size_bytes(uint_half_t max_subscribers, uint_half_t max_publishers) {
    // TODO: add overflow checks
    return sizeof(RealTimeInstanceData)                         // one common header
           + (sizeof(RealTimePublisherEntry) * max_publishers)  // each publisher needs a RealTimePublisherEntry
           + (sizeof(T_p) * per_publisher_pool_size(max_subscribers) * max_publishers);  // total number of T_p
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<RealTimeMessageBuffer<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                            uint_half_t max_subscribers,
                                                                            uint_half_t max_publishers,
                                                                            T_Args&&... args) {
    logging::debug("RealTimeMessageBuffer::init_at()");
    if (size_bytes < required_size_bytes(max_subscribers, max_publishers)) {
      // TODO: add proper errors
      logging::warn("RealTimeMessageBuffer::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    uint_t capacity = (max_subscribers + 2) * max_publishers;
    if (capacity > (std::numeric_limits<uint_half_t>::max())) {
      // TODO: error is that it is impossible to store the index of all elements in haf a uint_t
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    RealTimeInstanceData* header =
        std::construct_at(reinterpret_cast<RealTimeInstanceData*>(addr), max_subscribers, max_publishers, 0);

    uint_t expected_initialization_value = 0;
    if (!header->initialization_state.compare_exchange_weak(expected_initialization_value, 1,
                                                            std::memory_order_acq_rel)) {
      // TODO: already initialized or in initialization
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    std::span<RealTimePublisherEntry> per_publisher_headers(
        reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), max_publishers);
    for (auto& pp_header : per_publisher_headers) {
      std::construct_at(std::addressof(pp_header));
    }

    std::span<T_p> buffer(
        reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) + (sizeof(RealTimePublisherEntry) * max_publishers)),
        capacity);
    for (auto& elem : buffer) {
      std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
    }

    // TODO: handle history size here!
    expected_initialization_value = 1;
    if (!header->initialization_state.compare_exchange_weak(expected_initialization_value, 2,
                                                            std::memory_order_acq_rel)) {
      // TODO: something is off with initialization
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    return RealTimeMessageBuffer(header, per_publisher_headers, buffer);
  }

  static std::expected<RealTimeMessageBuffer, std::error_code> read_at(std::uintptr_t addr) {
    logging::debug("RealTimeMessageBuffer::read_at()");

    auto* header = reinterpret_cast<RealTimeInstanceData*>(addr);

    std::uint64_t capacity = (header->max_subscribers + 2) * header->max_publishers;

    for (int i = 0; i < 100 && header->initialization_state.load() == 1; ++i) {
      // wait for initialization to be finished (total of 1000ms)
      std::this_thread::sleep_for(10ms);
    }

    if (header->initialization_state.load(std::memory_order_acquire) != 2) {
      // TODO: error: not initialized
      logging::warn("RealTimeMessageBuffer::read_at: memory not initialized (using RealTimeMessageBuffer::init_at)");
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    std::span<RealTimePublisherEntry> pp_headers(
        reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), header->max_publishers);
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                 (sizeof(RealTimePublisherEntry) * header->max_publishers)),
                          capacity);

    return RealTimeMessageBuffer(header, pp_headers, buffer);
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<RealTimeMessageBuffer<T_p>, std::error_code> read_or_init_at(
      std::uintptr_t addr, std::size_t size_bytes, uint_t max_subscribers, uint_t max_publishers, T_Args&&... args) {
    logging::debug("RealTimeMessageBuffer::init_at()");
    if (size_bytes < required_size_bytes(max_subscribers, max_publishers)) {
      // TODO: add proper errors
      logging::warn("RealTimeMessageBuffer::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    uint_t capacity = (max_subscribers + 2) * max_publishers;
    if (capacity > (std::numeric_limits<uint_half_t>::max())) {
      // TODO: error is that it is impossible to store the index of all elements in haf a uint_t
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    RealTimeInstanceData* header =
        std::construct_at(reinterpret_cast<RealTimeInstanceData*>(addr), max_subscribers, max_publishers, 0);

    uint_t expected_initialization_value = 0;
    if (header->initialization_state.compare_exchange_weak(expected_initialization_value, 1,
                                                           std::memory_order_acq_rel)) {
      // initialize
      // TODO: write _m_initialize that is expected to be called with initialization_state == 1
      std::span<RealTimePublisherEntry> per_publisher_headers(
          reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), max_publishers);
      for (auto& pp_header : per_publisher_headers) {
        std::construct_at(std::addressof(pp_header));
      }

      std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                   (sizeof(RealTimePublisherEntry) * max_publishers)),
                            capacity);
      for (auto& elem : buffer) {
        std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
      }

      expected_initialization_value = 1;
      if (!header->initialization_state.compare_exchange_weak(expected_initialization_value, 2,
                                                              std::memory_order_acq_rel)) {
        // TODO: something is off with initialization
        return std::unexpected(std::error_code(1, std::system_category()));
      }
      return RealTimeMessageBuffer(header, per_publisher_headers, buffer);
    } else {
      // already initialized: read it
      return read_at(addr);
    }

    while (header->initialization_state.load(std::memory_order_acquire) == 1) {
      std::this_thread::sleep_for(10ms);
    }

    if (header->initialization_state.fetch_add(1) > 2) {
      // already initialized, just read it
      header->initialization_state.fetch_sub(1);
      return read_at(addr);
    } else {
      // initialize
      std::span<RealTimePublisherEntry> per_publisher_headers(
          reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), max_publishers);
      for (auto& pp_header : per_publisher_headers) {
        std::construct_at(std::addressof(pp_header));
      }

      std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                   (sizeof(RealTimePublisherEntry) * max_publishers)),
                            capacity);
      for (auto& elem : buffer) {
        std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
      }

      // TODO: handle history size here!
      uint_t expected_prev = 1;
      if (!header->initialization_state.compare_exchange_weak(expected_prev, 2, std::memory_order_acq_rel)) {
        // TODO: something is off with initialization
        return std::unexpected(std::error_code(1, std::system_category()));
      }
      return RealTimeMessageBuffer(header, per_publisher_headers, buffer);
    }
  }

 public:
  value_type& operator[](uint_half_t index) { return _buffer[index]; }

  const value_type& operator[](uint_half_t index) const { return _buffer[index]; }

  uint_half_t get_index(uint_half_t publisher_id, uint_half_t local_message_id) {
    return publisher_id * (local_message_id & (_h_wrap_around_value));
  }

  [[nodiscard]] uint_t size() const { return static_cast<uint_t>(_buffer.size()); }

  RealTimeInstanceData* common_header() { return _common_header; }
  RealTimePublisherEntry* per_publisher_header(uint_half_t publisher_id) {
    return std::addressof(_pp_headers[publisher_id]);
  }

 private:
  RealTimeMessageBuffer(RealTimeInstanceData* header, std::span<RealTimePublisherEntry> pp_headers,
                        std::span<value_type> queue_items)
      : _common_header(header), _pp_headers(pp_headers), _buffer(queue_items) {
    _h_wrap_around_value = per_publisher_pool_size(header->num_subscribers) - 1;
  }

 private:
  RealTimeInstanceData* _common_header;
  std::span<RealTimePublisherEntry> _pp_headers;
  std::span<value_type> _buffer;
  // internally used for wrap-around of local message id to index: per_publisher_pool_size(header->num_subscribers) - 1
  //  stored to avoid recomputations
  uint_half_t _h_wrap_around_value;
};

}  // namespace ipcpp::ps