/**
* Copyright 2025, Leon Freist (https://github.com/lfreist)
* Author: Leon Freist <freist.leon@gmail.com>
*
* This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/service.h>
#include <ipcpp/types.h>
#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>
#include <ipcpp/utils/string_identifier.h>

namespace ipcpp {

template <typename T_p, ServiceMode T_SM>
class Service<ServiceType::real_time_publish_subscribe, T_p, T_SM> {
  // TODO: set alignment to page size
  struct alignas(4096) _S_memory_layout {

  };

 public:
  struct Options {
    uint_half_t max_subscribers = 1;
    uint_half_t max_publishers = 1;
  };

 public:
  Service(StringIdentifier<internal::max_identifier_size> identifier, const Options& options = {}) : _identifier(std::move(identifier)), _options(options) {}

  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_publisher() {}
  std::expected<ps::RealTimePublisher<T_p>, std::error_code> create_subscriber() {}

 private:
  StringIdentifier<internal::max_identifier_size> _identifier;
  Options _options;
};

}