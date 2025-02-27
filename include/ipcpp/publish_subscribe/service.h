/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>
#include <ipcpp/service_shm.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/string_identifier.h>

#include <map>

#include "real_time_publisher.h"

namespace carry::ps {

struct PublisherEntry {
  uint_half_t id;
  std::int64_t creation_timestamp;
  std::uint64_t pid;
  bool is_online = false;

  alignas(std::hardware_destructive_interference_size) uint_half_t next_message_id = 0;
};

struct SubscriberEntry {
  uint_half_t id;
  std::int64_t creation_timestamp;
  std::uint64_t pid;
  bool is_online = false;
};

struct RealTimeServiceShm {
  static std::size_t required_size(uint_half_t num_publishers, uint_half_t num_subscribers) {
    return sizeof(RealTimeServiceShm) + (num_publishers * sizeof(PublisherEntry)) + (num_subscribers * sizeof(SubscriberEntry));
  }

  const ServiceShm service_shm;
  const uint_half_t max_publishers = 1;
  const uint_half_t max_subscribers = 1;

  const uint_half_t history_size = 0;

  // actually accessed by workers
  std::atomic<uint_half_t> num_publishers = 0;
  std::atomic<uint_half_t> num_subscribers = 0;

  alignas(std::hardware_destructive_interference_size) std::atomic<uint_t> latest_published =
      std::numeric_limits<uint_t>::max();

  std::span<PublisherEntry> get_publisher_slots() {
    return {reinterpret_cast<PublisherEntry*>(reinterpret_cast<std::uint8_t*>(this) + sizeof(RealTimeServiceShm)),
            max_publishers};
  }
  std::span<SubscriberEntry> get_subscriber_slots() {
    return {reinterpret_cast<SubscriberEntry*>(reinterpret_cast<std::uint8_t*>(this) + sizeof(RealTimeServiceShm) +
                                           (sizeof(PublisherEntry) * max_publishers)),
            max_subscribers};
  }
};

template <Scope T_s, PublishPolicy T_dm, typename T>
class Builder {
 public:
  struct ServiceOptions {};

 public:
  Builder(std::string identifier) : _identifier(std::move(identifier)) {
    // TODO: get shm and initialize it if not done yet
  }

  RealTimePublisher<T> create_publisher(const publisher::Options& options)
    requires(T_dm == PublishPolicy::real_time);

  RealTimeSubscriber<T> create_subscriber(const subscriber::Options& options)
    requires(T_dm == PublishPolicy::real_time);

  // TODO: factory functions for fifo subscribers

 private:
  std::string _identifier;
  ShmEntryPtr _service_shm = nullptr;
  ShmEntryPtr _data_shm = nullptr;
};

}  // namespace carry::ps