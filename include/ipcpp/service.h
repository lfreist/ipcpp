/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <cstring>

#include <ipcpp/publish_subscribe/service.h>

#include <ipcpp/service_shm.h>

namespace carry {

template <Scope T_s>
class Service {
 public:
  template <PublishPolicy T_dm, typename T>
  static std::expected<ps::Builder<T_s, T_dm, T>, std::error_code> PublishSubscribe(std::string identifier);

  // template <PublishPolicy T_dm, typename T>
  // static std::expected<RequestResponseBuilder<Scope, T_dm, T>, std::error_code> PublishSubscribe(std::string identifier);

  // template <PublishPolicy T_dm, typename T>
  // static std::expected<PipeBuilder<Scope, T_dm, T>, std::error_code> PublishSubscribe(std::string identifier);
};

}  // namespace carry