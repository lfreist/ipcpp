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
  uint_half_t id = 0;
  std::uint64_t pid = 0;
  std::int64_t creation_timestamp = -1;
  alignas(std::hardware_destructive_interference_size) uint_half_t next_local_message_id = 0;
  alignas(std::hardware_destructive_interference_size) bool is_online = false;

  [[nodiscard]] bool is_available() const { return id == 0 && pid == 0 && creation_timestamp == -1; }
};

struct RealTimeInstanceData {
  enum class InitializationState : uint_t {
    uninitialized = 0,
    in_initialization,
    initialized
  };

  RealTimeInstanceData(uint_half_t max_publishers_, uint_half_t max_subscribers_)
      : max_publishers(max_publishers_), max_subscribers(max_subscribers_) {}

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> num_subscribers = 0;
  alignas(std::hardware_destructive_interference_size) const uint_half_t max_subscribers = 0;

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> num_publishers = 0;
  alignas(std::hardware_destructive_interference_size) const uint_half_t max_publishers = 0;

  // this is the value that is actually polled by subscribers
  // first half is index, second half is local message id. the combination makes it unique since each publisher gets
  //  its index range
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> latest_published =
      std::numeric_limits<uint_t>::max();
  // if all publishers went down, final_published is set to latest_published and latest_published is set to 0xF...F
  alignas(std::hardware_destructive_interference_size) uint_t final_published = 0;

  alignas(std::hardware_destructive_interference_size) std::atomic<InitializationState> initialization_state = InitializationState::uninitialized;
};

/**
 * Memory Layout:
 * |----------------------|
 * | RealTimeInstanceData |
 * | PerPublisherHeader 0 |
 * | ...                  |
 * | PerPublisherHeader n |
 * | Message Buffer Start |
 * | ...                  |
 * | Message Buffer End   |
 * |----------------------|
 */
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
      // TODO: error is that it is impossible to store the index of all elements in half a uint_t
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    auto* header = std::construct_at(reinterpret_cast<RealTimeInstanceData*>(addr), max_subscribers, max_publishers);

    // set to in_initialization
    RealTimeInstanceData::InitializationState expected_initialization_value = RealTimeInstanceData::InitializationState::uninitialized;
    if (!header->initialization_state.compare_exchange_weak(expected_initialization_value, RealTimeInstanceData::InitializationState::in_initialization,
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

    expected_initialization_value = RealTimeInstanceData::InitializationState::in_initialization;
    if (!header->initialization_state.compare_exchange_weak(expected_initialization_value, RealTimeInstanceData::InitializationState::initialized,
                                                            std::memory_order_acq_rel)) {
      // TODO: something is off with initialization
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    return RealTimeMessageBuffer(header, per_publisher_headers, buffer);
  }

  static std::expected<RealTimeMessageBuffer, std::error_code> read_at(std::uintptr_t addr, std::chrono::milliseconds timeout = 1000ms) {
    logging::debug("RealTimeMessageBuffer::read_at()");

    auto* header = reinterpret_cast<RealTimeInstanceData*>(addr);

    {  // wait for RealTimeInstanceData to be initialized or timeout
      using clock = std::chrono::high_resolution_clock;
      auto start = clock::now();
      while (true) {
        if (header->initialization_state.load(std::memory_order_acquire) ==
            RealTimeInstanceData::InitializationState::initialized) {
          break;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start) >=
            timeout) {
            logging::warn("RealTimeMessageBuffer::read_at: memory not initialized (using RealTimeMessageBuffer::init_at)");
            // TODO: error is timeout
          return std::unexpected(std::error_code(1, std::system_category()));
        }
        std::this_thread::sleep_for(1ms);
      }
    }

    std::uint64_t capacity = (header->max_subscribers + 2) * header->max_publishers;

    std::span<RealTimePublisherEntry> pp_headers(
        reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), header->max_publishers);
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                 (sizeof(RealTimePublisherEntry) * header->max_publishers)),
                          capacity);

    return RealTimeMessageBuffer(header, pp_headers, buffer);
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
      : _common_header(header),
        _pp_headers(pp_headers),
        _buffer(queue_items),
        _h_wrap_around_value(RealTimeMessageBuffer::per_publisher_pool_size(header->num_subscribers) - 1) {}

 private:
  RealTimeInstanceData* _common_header;
  std::span<RealTimePublisherEntry> _pp_headers;
  std::span<value_type> _buffer;
  /// internally used for wrap-around of local message id to index: per_publisher_pool_size(header->num_subscribers) - 1
  // TODO: make const
  uint_half_t _h_wrap_around_value;
};

}  // namespace ipcpp::ps