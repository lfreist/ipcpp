/**
* Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp {

enum class ServiceMode {
  local,
  ipc
};

enum class ServiceType {
  real_time_publish_subscribe,
  fifo_request_response,
  pipe,
  event
};

template <ServiceType T_ST, typename T_p, ServiceMode T_SC>
class Service {};

}