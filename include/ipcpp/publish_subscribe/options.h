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

namespace ipcpp::publish_subscribe {

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

/**
* PublishPolicy defines when data are actually published
*/
enum class PublishPolicy {
  always,
  subscribed,  // only publish data when a subscription is active.
               // Note: If a history of size >= 1 is configured, PublishPolicy::subscribed is equal to always
  timed,       // only published when a timer has expired since the last publish
};


struct Options {
  BackpressurePolicy backpressure_policy = BackpressurePolicy::block;
  PublishPolicy publish_policy = PublishPolicy::always;
  std::chrono::nanoseconds timed_publish_interval = 0ms;
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
* ReceivePolicy defines what message is read on on_receive calls.
*/
enum class ReceivePolicy {
  all,     // read oldest not yet read message -> receive all messages in FIFO manner
  latest,  // only read the next published message -> live stream
  history  // read oldest nmot yet read message in history (there are few usecases for this!)
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
  ReceivePolicy receive_policy = ReceivePolicy::all;
  OnSubscribeReceivePolicy on_subscribe_policy = OnSubscribeReceivePolicy::latest;
  OnInvalidatedReadPolicy on_invalidated_read_policy = OnInvalidatedReadPolicy::error;
};

}

}