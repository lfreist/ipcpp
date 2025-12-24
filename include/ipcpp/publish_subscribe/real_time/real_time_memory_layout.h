/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/options.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/atomic.h>
#include <ipcpp/utils/ip_lock.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/numeric.h>
#include <ipcpp/utils/system.h>
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

struct ProcessData {
  std::uint64_t pid = 0;
  std::int64_t creation_timestamp = -1;
};

struct RealTimeSubscriberEntry {
  alignas(std::hardware_destructive_interference_size) ProcessData process_data;
  const uint_half_t id = std::numeric_limits<uint_half_t>::max();

  RealTimeSubscriberEntry() = default;
  explicit RealTimeSubscriberEntry(uint_half_t subscriber_id) : RealTimeSubscriberEntry(subscriber_id, 100ms) {}
  RealTimeSubscriberEntry(uint_half_t subscriber_id, std::chrono::milliseconds heartbeat_interval)
      : id(subscriber_id),
        process_data{.pid = static_cast<std::uint64_t>(utils::system::get_pid()),
                     .creation_timestamp = utils::timestamp()} {}

  [[nodiscard]] bool is_available() const {
    return (id == 0 && process_data.pid == 0 && process_data.creation_timestamp == -1) || !is_alive();
  }
  [[nodiscard]] bool is_alive() const { return utils::system::is_process_alive(process_data.pid); }
};

struct RealTimePublisherEntry {
  alignas(std::hardware_destructive_interference_size) ProcessData process_data;
  alignas(std::hardware_destructive_interference_size) uint_half_t next_local_message_id = 0;
  const uint_half_t id = std::numeric_limits<uint_half_t>::max();

  RealTimePublisherEntry() = default;
  explicit RealTimePublisherEntry(uint_half_t publisher_id) : RealTimePublisherEntry(publisher_id, 100ms) {}
  RealTimePublisherEntry(uint_half_t publisher_id, std::chrono::milliseconds heartbeat_interval)
      : id(publisher_id),
        process_data{.pid = static_cast<std::uint64_t>(getpid()), .creation_timestamp = utils::timestamp()} {}

  [[nodiscard]] bool is_available() const {
    return (id == 0 && process_data.pid == 0 && process_data.creation_timestamp == -1) || !is_alive();
  }
  [[nodiscard]] bool is_alive() const { return utils::system::is_process_alive(process_data.pid); }
};

struct RealTimeInstanceData {
  explicit RealTimeInstanceData(const Options<Mode::RealTime>& options) : options(options) {}

  /// running message id
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> next_message_id =
      std::numeric_limits<uint_t>::max();
  /// global message index in message_buffer
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> latest_published_idx =
      std::numeric_limits<uint_t>::max();
  /// initialization state to avoid concurrent initializations
  alignas(std::hardware_destructive_interference_size) std::atomic<InitializationState> initialization_state =
      InitializationState::uninitialized;

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> next_publisher_id = 0;
  alignas(std::hardware_destructive_interference_size) std::atomic<uint_half_t> next_subscriber_id = 0;

  const Options<Mode::RealTime> options;
};

/**
 * number of publishers: n
 * number of subscribers: m
 * Memory Layout:
 * |---------------------------------------------------------|
 * | RealTimeInstanceData                                    |
 * | PerPublisherHeader 0                                    |
 * | ...                                                     |
 * | PerPublisherHeader n                                    |
 * | PerSubscriberHeader 0                                   |
 * | ...                                                     |
 * | PerSubscriberHeader m                                   |
 * | Message Buffer 0                                        |
 * | ...                                                     |
 * | Message Buffer n * numeric::ceil_to_power_of_two(m + 2) |
 * |---------------------------------------------------------|
 */
template <typename T_p>
class RealTimeMessageBuffer {
 public:
  typedef std::remove_cvref_t<T_p> value_type;

 public:
  static uint_half_t per_publisher_pool_size(const Options<Mode::RealTime>& options) {
    assert(options.max_subscribers <= std::numeric_limits<uint_half_t>::max() - 2);
    // rounded to power of two to allow fast wrap-around of index overflows. This is necessary because we track a local
    // message id from which we need to access a T_p in the publishers pool.
    return numeric::ceil_to_power_of_two((options.max_subscribers * options.max_concurrent_acquires) + 2);
  }

  static uint_t required_size_bytes(const Options<Mode::RealTime>& options) {
    return sizeof(RealTimeInstanceData)                                 // one common header
           + (sizeof(RealTimePublisherEntry) * options.max_publishers)  // each publisher needs a RealTimePublisherEntry
           + (sizeof(RealTimeSubscriberEntry) *
              options.max_subscribers)  // each subscriber needs a RealTimeSubscriberEntry
           + (sizeof(T_p) * per_publisher_pool_size(options) * options.max_publishers);  // total number of T_p
  }

  static uint_t message_buffer_size(const Options<Mode::RealTime>& options) {
    return RealTimeMessageBuffer::per_publisher_pool_size(options) * options.max_publishers;
  }

  template <typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static std::expected<RealTimeMessageBuffer<T_p>, std::error_code> init_at(std::uintptr_t addr, std::size_t size_bytes,
                                                                            Options<Mode::RealTime> options,
                                                                            T_Args&&... args) {
    logging::debug("RealTimeMessageBuffer::init_at()");
    if (size_bytes < required_size_bytes(options)) {
      // TODO: add proper errors
      logging::warn("RealTimeMessageBuffer::init_at: provided size too small");
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    std::uint64_t capacity = RealTimeMessageBuffer::message_buffer_size(options);
    if (capacity > (std::numeric_limits<uint_half_t>::max())) {
      // TODO: error is that it is impossible to store the index of all elements in uint_half_t
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    auto* header = std::construct_at(reinterpret_cast<RealTimeInstanceData*>(addr), options);

    // set to in_initialization
    InitializationState expected_initialization_value = InitializationState::uninitialized;
    if (!header->initialization_state.compare_exchange_weak(
            expected_initialization_value, InitializationState::in_initialization, std::memory_order_acq_rel)) {
      // TODO: already initialized or in initialization
      return std::unexpected(std::error_code(1, std::system_category()));
    }

    std::span<RealTimePublisherEntry> per_publisher_headers(
        reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), options.max_publishers);
    for (auto& pp_header : per_publisher_headers) {
      std::construct_at(std::addressof(pp_header));
    }

    std::span<RealTimeSubscriberEntry> per_subscriber_headers(
        reinterpret_cast<RealTimeSubscriberEntry*>(addr + sizeof(RealTimeInstanceData) +
                                                   (sizeof(RealTimePublisherEntry) * options.max_publishers)),
        options.max_subscribers);
    for (auto& ps_header : per_subscriber_headers) {
      std::construct_at(std::addressof(ps_header));
    }

    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                 (sizeof(RealTimePublisherEntry) * options.max_publishers) +
                                                 (sizeof(RealTimeSubscriberEntry) * options.max_subscribers)),
                          capacity);
    for (auto& elem : buffer) {
      std::construct_at(std::addressof(elem), std::forward<T_Args>(args)...);
    }

    expected_initialization_value = InitializationState::in_initialization;
    if (!header->initialization_state.compare_exchange_weak(
            expected_initialization_value, InitializationState::initialized, std::memory_order_acq_rel)) {
      // TODO: something is off with initialization
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    return RealTimeMessageBuffer(header, per_publisher_headers, per_subscriber_headers, buffer);
  }

  static std::expected<RealTimeMessageBuffer, std::error_code> read_at(std::uintptr_t addr,
                                                                       std::chrono::milliseconds timeout = 1000ms) {
    logging::debug("RealTimeMessageBuffer::read_at()");

    auto* header = reinterpret_cast<RealTimeInstanceData*>(addr);

    {  // wait for RealTimeInstanceData to be initialized or timeout
      using clock = std::chrono::high_resolution_clock;
      auto start = clock::now();
      while (true) {
        if (header->initialization_state.load(std::memory_order_acquire) == InitializationState::initialized) {
          break;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start) >= timeout) {
          logging::warn(
              "RealTimeMessageBuffer::read_at: memory not initialized (using RealTimeMessageBuffer::init_at)");
          // TODO: error is timeout
          return std::unexpected(std::error_code(1, std::system_category()));
        }
        std::this_thread::sleep_for(1ms);
      }
    }

    std::uint64_t capacity = RealTimeMessageBuffer::message_buffer_size(header->options);

    std::span<RealTimePublisherEntry> pp_headers(
        reinterpret_cast<RealTimePublisherEntry*>(addr + sizeof(RealTimeInstanceData)), header->options.max_publishers);
    std::span<RealTimeSubscriberEntry> ps_headers(
        reinterpret_cast<RealTimeSubscriberEntry*>(addr + sizeof(RealTimeInstanceData) +
                                                   (sizeof(RealTimePublisherEntry) * header->options.max_publishers)),
        header->options.max_subscribers);
    std::span<T_p> buffer(reinterpret_cast<T_p*>(addr + sizeof(RealTimeInstanceData) +
                                                 (sizeof(RealTimePublisherEntry) * header->options.max_publishers) +
                                                 (sizeof(RealTimeSubscriberEntry) * header->options.max_subscribers)),
                          capacity);

    return RealTimeMessageBuffer(header, pp_headers, ps_headers, buffer);
  }

 public:
  value_type& operator[](uint_half_t index) { return _buffer[index]; }

  const value_type& operator[](uint_half_t index) const { return _buffer[index]; }

  uint_half_t get_index(uint_half_t publisher_idx, uint_half_t local_message_id) {
    return publisher_idx * per_publisher_pool_size(_common_header->options) + (local_message_id & _h_wrap_around_value);
  }

  value_type& at(uint_half_t publisher_id, uint_half_t local_message_id) {
    return this->operator[](get_index(publisher_id, local_message_id));
  }

  const value_type& at(uint_half_t publisher_id, uint_half_t local_message_id) const {
    return this->operator[](get_index(publisher_id, local_message_id));
  }

  [[nodiscard]] uint_t size() const { return static_cast<uint_t>(_buffer.size()); }

  RealTimeInstanceData* common_header() { return _common_header; }
  RealTimePublisherEntry* per_publisher_header(uint_half_t publisher_idx) {
    return std::addressof(_publisher_entries[publisher_idx]);
  }
  RealTimeSubscriberEntry* per_subscriber_header(uint_half_t subscriber_idx) {
    return std::addressof(_subscriber_entries[subscriber_idx]);
  }

 private:
  RealTimeMessageBuffer(RealTimeInstanceData* header, std::span<RealTimePublisherEntry> pp_headers,
                        std::span<RealTimeSubscriberEntry> ps_headers, std::span<value_type> queue_items)
      : _common_header(header),
        _publisher_entries(pp_headers),
        _subscriber_entries(ps_headers),
        _buffer(queue_items),
        _h_wrap_around_value(RealTimeMessageBuffer::per_publisher_pool_size(header->options) - 1) {}

 private:
  RealTimeInstanceData* _common_header;
  std::span<RealTimePublisherEntry> _publisher_entries;
  std::span<RealTimeSubscriberEntry> _subscriber_entries;
  std::span<value_type> _buffer;
  /// internally used for wrap-around of local message id to index: per_publisher_pool_size(header->num_subscribers) - 1
  // TODO: make const
  uint_half_t _h_wrap_around_value;
};

}  // namespace ipcpp::ps