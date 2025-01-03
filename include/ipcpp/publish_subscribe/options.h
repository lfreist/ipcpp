/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>
#include <chrono>

using namespace std::chrono_literals;

namespace ipcpp::ps {

namespace publisher {

/**
* BackpressurePolicy defines how message buffer overflows are handled. Message buffer overflows may occur when a
*  subscriber is too slow and uses the subscriber::ReceivePolicy::ALL.
*/
enum class BackpressurePolicy {
  block,          // the publisher blocks until the next slot is free
  remove_oldest,  // the next slot in the message buffer is overwritten and its previous message gets invalidated
                  //  note: it is likely that the slow subscriber is currently reading this message. In this case, the
                  //   next slot gets invalidated so that the subscriber uses its subscriber::OnInvalidatedReadPolicy
                  //   when trying to read the next message. Once the subscriber has released the slot, it gets
                  //   overwritten. This means, that the publisher may block in this mode aswell!
};


struct Options {
  BackpressurePolicy backpressure_policy = BackpressurePolicy::block;
  std::size_t queue_capacity = 64;
  std::size_t history_capacity = 0;
  std::size_t max_num_observers = 0;  // 0 is uncapped
};

}

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
  latest,   // start with next published message (refers to the next published message at subscription, NOT at first on_receive call)!
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
  WaitStrategy wait_strategy = WaitStrategy::adaptive;
  OnSubscribeReceivePolicy on_subscribe_policy = OnSubscribeReceivePolicy::latest;
  OnInvalidatedReadPolicy on_invalidated_read_policy = OnInvalidatedReadPolicy::error;
};

}

}