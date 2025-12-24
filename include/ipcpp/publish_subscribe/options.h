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

enum class Mode {
  RealTime,      // only latest message is available to all subscribers
  Sequence,      // all messages are published to all subscribers
  MessageQueue,  // each message is published to exactly/at most one subscriber (depending on BackpressurePolicy)
};

enum class RealTimeSubscriptionMode {
  Volatile,  // subscriber receives first message published *after* subscription
  Latched,   // subscriber receives latest published message *at* subscription
};

enum class BackpressurePolicy {
  Blocking,       // the publisher blocks until the next slot is free
  ReturnError,    // an error value is returned
  ReplaceOldest,  // oldest message not occupied is overwritten
};

template <Mode T_p>
struct Options;

template <>
struct Options<Mode::RealTime> {
  uint_half_t max_publishers = 1;
  uint_half_t max_subscribers = 1;
  uint_half_t max_concurrent_acquires = 1;
};

enum class InitializationState : uint_t { uninitialized = 0, in_initialization, initialized };

}  // namespace ipcpp::ps