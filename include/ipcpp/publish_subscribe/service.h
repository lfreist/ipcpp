/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>
#include <ipcpp/service.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/string_identifier.h>

namespace carry::publish_subscribe {

template <typename T_p, ServiceMode T_sm, PublishPolicy T_pp>
class Service : public Service_Interface {
  struct ShmPublisherEntry {
    static std::size_t required_size() { return sizeof(ShmPublisherEntry); }

    uint_half_t id;
    std::int64_t creation_timestamp;
    std::uint64_t pid;
    char identifier[internal::max_identifier_size];
  };

  struct ShmSubscriberEntry {
    static std::size_t required_size() { return sizeof(ShmSubscriberEntry); }

    uint_half_t id;
    std::int64_t creation_timestamp;
    std::uint64_t pid;
    // TODO: add list of publisher filter
  };

  // TODO: set alignment to page size
  struct ShmConnectionLayout {
    static std::size_t required_size(uint_half_t max_subscribers, uint_half_t max_publishers) {
      return sizeof(ShmConnectionLayout) +
             (max_subscribers * ShmSubscriberEntry::required_size())   // list of subscriber entries
             + (max_publishers * ShmPublisherEntry::required_size());  // list of publisher_entries
    }

    ShmConnectionLayout(const StringIdentifier& identifier_, uint_half_t max_subscribers, uint_half_t max_publishers) : max_publishers(max_publishers), max_subscribers(max_subscribers), identifier(0) {
      subscriber_list_offset = reinterpret_cast<std::uintptr_t>(this) + sizeof(*this);
      publisher_list_offset = reinterpret_cast<std::uintptr_t>(this) + sizeof(*this) + (max_subscribers * ShmSubscriberEntry::required_size());
      for (int i = 0; i < internal::max_identifier_size; ++i) {
        if (i > identifier_.size()) {
          break;
        } else {
          identifier[i] = identifier_.name()[i];
        }
      }
    }

    std::atomic<uint8_t> initialization_state = 0;
    const bool is_consistent = false;  // no unlink if true
    const std::uint64_t version_tag = 0;
    std::atomic<uint_t> reference_counter = 0;
    char identifier[internal::max_identifier_size];
    const uint_half_t max_subscribers;
    const uint_half_t max_publishers;

    /// offset from this
    std::ptrdiff_t subscriber_list_offset;
    /// offset from this
    std::ptrdiff_t publisher_list_offset;

    std::span<ShmSubscriberEntry> subscriber_list() {
      return {reinterpret_cast<ShmSubscriberEntry*>(reinterpret_cast<std::uintptr_t>(this) + subscriber_list_offset), max_subscribers};
    }

    std::span<ShmSubscriberEntry> publisher_list() {
      return {reinterpret_cast<ShmPublisherEntry*>(reinterpret_cast<std::uintptr_t>(this) + publisher_list_offset), max_publishers};
    }
  };

 public:
  struct Options {
    uint_half_t max_subscribers = 1;
    uint_half_t max_publishers = 1;
  };

 public:
  Service(std::string identifier, const Options& options)
      : _identifier(std::move(identifier)), _options(options) {
    _m_open_shm();
    _m_init_message_buffer();
  }

  explicit Service(StringIdentifier identifier)
      : _identifier(std::move(identifier)) {
    _m_open_shm();
    _m_init_message_buffer();
  }

 public:
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_publisher() {

    return ps::RealTimePublisher<T_p>();
  }
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_subscriber() {
    return std::unexpected(std::error_code());
  }

  [[nodiscard]] const StringIdentifier& get_identifier() const {
    return _identifier;
  }

 private:
  std::string _m_shm_connection_name() {
    return std::format("{}.rt_publish_subscribe.carry.connection", _identifier.name());
  }

  std::string _m_shm_data_name() { return std::format("{}.rt_publish_subscribe.carry.data", _identifier.name()); }

  /**
   * open service shm: if its initialized, just use it, else initialize it
   */
  void _m_open_shm() {
    auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::create(_identifier, ShmConnectionLayout::required_size());
    if (!e_mm) {
      throw std::runtime_error(std::format("Failed to open/create shared memory (connection) for Service '{}': TODO: print error", _identifier.name()));
    }
    _service_shm.emplace(std::move(e_mm.value()));
    auto shm_connection = get_shm_entry(_m_shm_connection_name(), ShmConnectionLayout::required_size());
    if (!shm_connection) {
      throw std::runtime_error(std::format("Failed to open/create shared memory (connection) for Service '{}': TODO: print error", _identifier.name()));
    }
    _shm_connection = std::move(shm_connection.value());
    auto shm_data = get_shm_entry(_m_shm_data_name(), ShmConnectionLayout::required_size());
    if (!shm_data) {
      throw std::runtime_error(std::format("Failed to open/create shared memory (data) for Service '{}': TODO: print error", _identifier.name()));
    }
    _shm_data = std::move(shm_data.value());
  }

  void _m_init_shm_connection_layout() {
    // TODO: handle initialization more robust:
    //  1. check if already initialized, if so just reinterpret, if not use std::construct_at
    //  2. check if another process is initializing it at the moment:
    //     - use file access mode: if a process initializes the shm, dont allow read access on the shm fd
    _shm_connection_layout = std::construct_at(_shm_connection->shm().addr(), _identifier, _options.max_subscribers, _options.max_publishers);
    InitializationState expected = InitializationState::uninitialized;
    if (_shm_connection_layout->initialization_state.compare_exchange_weak(expected, InitializationState::initialization_in_progress)) {
      // initialize

      expected = InitializationState::initialization_in_progress;
      if (_shm_connection_layout->initialization_state.compare_exchange_weak(expected, InitializationState::initialized)) {
        return;
      } else {
        throw std::runtime_error("Error initializing connection layout");
      }
    } else {
      // wait 1s for layout being initialized
      for (int i = 0; i < 10; ++i) {
        if (_shm_connection_layout->initialization_state.load() == InitializationState::initialized) {
          return;
        }
        std::this_thread::sleep_for(100ms);
      }
      throw std::runtime_error("Error: connection layout stuck at initialization for 1s");
    }
  }

  void _m_init_message_buffer() {
    auto e_buffer = ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type>::read_at(_shm_data->shm().addr());
    if (!e_buffer) {
      e_buffer = ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type>::init_at(_shm_data->shm().addr(), _shm_data->shm().size(),
                                                                                                _options.max_subscribers, _options.max_publishers);
    }
    if (!e_buffer) {
      throw std::runtime_error(std::format("Service '{}': Failed to initialize message_buffer", _identifier.name()));
    }
  }

 private:
  StringIdentifier _identifier;
  Options _options;
  ShmEntryPtr _shm_connection;
  ShmEntryPtr _shm_data;
  /// message buffer built at _shm_data
  ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type> _message_buffer;
  /// connection data built at _shm_connection
  ShmConnectionLayout* _shm_connection_layout;
};

}  // namespace carry