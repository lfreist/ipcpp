/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>
#include <ipcpp/service.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/string_identifier.h>

namespace ipcpp {

template <typename T_p, ServiceMode T_SM>
class Service<ServiceType::real_time_publish_subscribe, T_p, T_SM> : public std::enable_shared_from_this<Service<ServiceType::real_time_publish_subscribe, T_p, T_SM>> {
  struct _S_publisher_entry {
    static std::size_t required_size() { return sizeof(_S_publisher_entry); }

    uint_half_t id;
    std::int64_t creation_timestamp;
    std::uint64_t pid;
    char identifier[internal::max_identifier_size];
  };

  struct _S_subscriber_entry {
    static std::size_t required_size() { return sizeof(_S_subscriber_entry); }

    uint_half_t id;
    std::int64_t creation_timestamp;
    std::uint64_t pid;
    // TODO: add list of publisher filter
  };

  // TODO: set alignment to page size
  struct _S_memory_layout {
    static std::size_t required_size(uint_half_t max_subscribers, uint_half_t max_publishers) {
      return sizeof(_S_memory_layout) +
             (max_subscribers * _S_subscriber_entry::required_size())   // list of subscriber entries
             + (max_publishers * _S_publisher_entry::required_size());  // list of publisher_entries
    }

    _S_memory_layout(uint_half_t max_subscribers, uint_half_t max_publishers) : max_publishers(max_publishers), max_subscribers(max_subscribers) {}

    std::atomic<uint_t> reference_counter = 0;
    char identifier[internal::max_identifier_size];
    const uint_half_t max_subscribers;
    const uint_half_t max_publishers;

    /// offset from this
    std::ptrdiff_t subscriber_list_offset;
    /// offset from this
    std::ptrdiff_t publisher_list_offset;

   public:
    std::span<_S_subscriber_entry> subscriber_list() {
      return {reinterpret_cast<_S_subscriber_entry*>(reinterpret_cast<std::uint8_t*>(this) + subscriber_list_offset), max_subscribers};
    }

    std::span<_S_subscriber_entry> publisher_list() {
      return {reinterpret_cast<_S_publisher_entry*>(reinterpret_cast<std::uint8_t*>(this) + publisher_list_offset), max_publishers};
    }
  };

 public:
  struct Options {
    uint_half_t max_subscribers = 1;
    uint_half_t max_publishers = 1;
  };

 private:
  explicit Service(std::string identifier, const Options& options = {})
      : _identifier(std::move(identifier)), _options(options) {
    auto shm_entry = get_shm_entry(_m_shm_connection_name());
    if (!shm_entry) {
      throw std::runtime_error("Failed to open shared memory");
    }
    _shm_entry = std::move(shm_entry.value());
  }

 public:
  std::shared_ptr<Service<ServiceType::real_time_publish_subscribe, T_p, T_SM>> build(std::string identifier, const Options& options = {}) {
    return std::make_shared<Service<ServiceType::real_time_publish_subscribe, T_p, T_SM>>(Service(std::move(identifier), options));
  }

 public:
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_publisher() {
    return ps::RealTimePublisher<T_p>();
  }
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_subscriber() {
    return std::unexpected(std::error_code());
  }

 private:
  std::string _m_shm_connection_name() {
    return std::format("{}.rt_publish_subscribe.ipcpp.connection", _identifier.name());
  }

  std::string _m_shm_data_name() { return std::format("{}.rt_publish_subscribe.ipcpp.data", _identifier.name()); }

 private:
  StringIdentifier _identifier;
  Options _options;
  std::shared_ptr<ShmRegistryEntry> _shm_entry;
};

}  // namespace ipcpp