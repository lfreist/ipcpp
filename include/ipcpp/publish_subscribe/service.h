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

#include <map>

namespace carry {

template <Scope T_s>
class Service<T_s, ServiceType::publish_subscribe> {
 public:
  enum class DeliveryMode {
    fifo,
    real_time
  };

 public:
  static std::expected<Service, std::error_code> named(std::string_view identifier);

 public:
  template <DeliveryMode T_dm, typename T>
  std::expected<ps::Subscriber<T>, std::error_code> create_subscriber(std::string_view identifier, Subscriber::Config config);

 private:
};

}  // namespace carry