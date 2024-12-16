/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::publish_subscribe {

template <typename T_Observer>
class Subscriber_I {
protected:
 T_Observer _observer;

 public:
  virtual ~Subscriber_I() = default;
  Subscriber_I(Subscriber_I&& other) noexcept : _observer(std::move(other._observer)) {}

 virtual auto subscribe() -> decltype(_observer.subscribe()) = 0;

 virtual auto cancel_subscription() -> decltype(_observer.cancel_subscription()) = 0;

 virtual auto pause_subscription() -> decltype(_observer.pause_subscription()) = 0;

 virtual auto resume_subscription() -> decltype(_observer.resume_subscription()) = 0;

 protected:
  explicit Subscriber_I(T_Observer&& observer) : _observer(std::move(observer)){}
};

}  // namespace ipcpp::publish_subscribe