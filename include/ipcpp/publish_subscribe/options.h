/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/types.h>

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
  block,          // the publisher blocks until the next slot is free
  remove_oldest,  // the next non-locked-slot in the message buffer is overwritten
};

struct Options {
  BackpressurePolicy backpressure_policy = BackpressurePolicy::remove_oldest;
  uint_t history_capacity = 0;
  uint_half_t max_num_observers = 1;
  uint_half_t max_num_publishers = 1;
};

}  // namespace publisher

namespace subscriber {

/**
 * WaitStrategy defines how the subscriber is waiting for new messages to be published.
 */
enum class WaitStrategy {
  polling,   // always poll for new messages
  blocking,  // always block when waiting for new messages
  adaptive,  // use polling or blocking determined by heuristics
  hybrid     // poll for a specific time, then swap to blocking if no message was received
};

/**
 * OnSubscribeReceivePolicy defines what message is read first after subscribing.
 */
enum class OnSubscribeReceivePolicy {
  latest,   // start with next published message (refers to the next published message at subscription, NOT at first
            // on_receive call)!
  history,  // start with history in FIFO manner
};

/**
 * OnInvalidatedReadPolicy defines the policy used when a subscriber receives a message_id that was invalidated.
 *  this may occur if a subscriber is too slow and the publisher has overwritten data that were not yet read by the
 *  subscriber.
 */
enum class OnInvalidatedReadPolicy {
  error,            // throw an error (error::invalidated_message_read)
  skip_one,         // skip the current message
  skip_to_latest,   // continue with reading next published message
  skip_to_history,  // continue with reading history (FIFO)
};

struct Options {
  WaitStrategy wait_strategy = WaitStrategy::polling;
  OnSubscribeReceivePolicy on_subscribe_policy = OnSubscribeReceivePolicy::latest;
  OnInvalidatedReadPolicy on_invalidated_read_policy = OnInvalidatedReadPolicy::error;
};

}  // namespace subscriber

}  // namespace ipcpp::ps