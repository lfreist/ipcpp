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

/**
 * Idee:
 *
 * auto publisher = Service<ipc>("identifier").create_publisher<real_time>("pub_identifier", options);
 * auto subscriber = Service<ipc>("identifier").create_subscriber<real_time>(options);
 *
 * subscriber.subscribe("pub_identifier");
 * publisher.~Publisher();  // calls Service<ipc
 *
 * implement Service as templated function that returns a ServiceClass queried from a static registry.
 * shm is queried in create_publisher/subscriber methods allowing different shm identifies for pub/sub, req/res, ...
 *
 * Architecture: keep shm registry and query the shm on Service construction and reinterpret it as intended.
 *               pass "identifier" to publishers and they can construct a Service on their destruction and unregister
 *               themselfes
 *
 * @tparam T_p
 * @tparam T_sm
 * @tparam T_pp
 */
template <typename T_p, ServiceMode T_sm = ServiceMode::ipc, PublishPolicy T_pp = PublishPolicy::real_time>
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

    ShmConnectionLayout(const StringIdentifier& identifier_, uint_half_t max_subscribers, uint_half_t max_publishers)
        : max_publishers(max_publishers), max_subscribers(max_subscribers), identifier(0) {
      subscriber_list_offset = reinterpret_cast<std::uintptr_t>(this) + sizeof(*this);
      publisher_list_offset = reinterpret_cast<std::uintptr_t>(this) + sizeof(*this) +
                              (max_subscribers * ShmSubscriberEntry::required_size());
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
      return {reinterpret_cast<ShmSubscriberEntry*>(reinterpret_cast<std::uintptr_t>(this) + subscriber_list_offset),
              max_subscribers};
    }

    std::span<ShmSubscriberEntry> publisher_list() {
      return {reinterpret_cast<ShmPublisherEntry*>(reinterpret_cast<std::uintptr_t>(this) + publisher_list_offset),
              max_publishers};
    }
  };

 public:
  struct Options {
    uint_half_t max_subscribers = 1;
    uint_half_t max_publishers = 1;
  };

 public:
  Service(std::string identifier, const Options& options)
      : _identifier(std::move(identifier)),
        _options(options),
        Service_Interface(
            _m_open_or_create_shm(std::format("{}.rt_ps.carry.connection", _identifier),
                                  ShmConnectionLayout::required_size()),
            _m_open_or_create_shm(
                std::format("{}.rt_ps.carry.connection", _identifier),
                ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type>::required_size_bytes(
                    _options.max_subscribers, _options.max_publishers))) {
    _m_init_service_buffer();
    _m_init_message_buffer();
  }

  explicit Service(std::string identifier) : Service(std::move(identifier), {}) {}

 public:
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_publisher() { return ps::RealTimePublisher<T_p>(); }
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_subscriber() {
    return std::unexpected(std::error_code());
  }

 private:
  std::string _m_shm_connection_name() { return std::format("{}.rt_publish_subscribe.carry.connection", _identifier); }

  std::string _m_shm_data_name() { return std::format("{}.rt_publish_subscribe.carry.data", _identifier); }

  /**
   * open service shm: if its initialized, just use it, else initialize it
   */
  static shm::MappedMemory<shm::MappingType::SINGLE>&& _m_open_or_create_shm(const std::string& identifier,
                                                                             std::size_t size) {
    auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::open(identifier, AccessMode::WRITE);
    if (!e_mm) {
      e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::create(identifier, size);
      if (!e_mm) {
        throw std::runtime_error(
            std::format("Failed to open/create shared memory '{}': TODO: print error", identifier));
      }
    }
    if (e_mm->size() < size) {
      throw std::runtime_error(
          std::format("Failed to open shared memory '{}': Shared memory exists but its size ({}) is smaller than the "
                      "required size ({})",
                      identifier, e_mm->size(), size));
    }
    return std::move(e_mm.value());
  }

  void _m_init_service_buffer() {
    // TODO: add file lock, check if initialized or not ...
  }

  void _m_init_message_buffer() {
    // TODO: add file lock, check if initialized or not ...
    auto e_buffer = ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type>::read_at(_data_shm.addr());
    if (!e_buffer) {
      e_buffer = ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type>::init_at(
          _data_shm.addr(), _data_shm.size(), _options.max_subscribers, _options.max_publishers);
    }
    if (!e_buffer) {
      throw std::runtime_error(std::format("Service '{}': Failed to initialize message_buffer", _identifier));
    }
  }

 private:
  std::string _identifier;
  Options _options;
  /// message buffer built at _shm_data
  ps::message_buffer<typename ps::RealTimePublisher<T_p>::message_type> _message_buffer;
  /// connection data built at _shm_connection
  ShmConnectionLayout* _shm_connection_layout;
};

}  // namespace carry::publish_subscribe