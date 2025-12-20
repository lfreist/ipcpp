/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/types.h>

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace ipcpp::ps {

namespace publisher {

/**
 * BackpressurePolicy defines how message buffer overflows are handled. Message buffer overflows may occur when a
 *  subscriber is too slow and uses the subscriber::ReceivePolicy::ALL.
 */
enum class BackpressurePolicy {
  block,  // the publisher blocks until the next slot is free
  error,  // an error value is returned
};

struct Options {
  BackpressurePolicy backpressure_policy = BackpressurePolicy::error;
  uint_half_t max_num_observers = 1;
  uint_half_t max_num_publishers = 1;
};

}  // namespace publisher


}  // namespace ipcpp::ps