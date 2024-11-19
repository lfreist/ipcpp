/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::publish_subscribe {

template <typename T, typename ObserverT>
class Subscriber_I {
 public:
  virtual ~Subscriber_I() = default;
  Subscriber_I(Subscriber_I&& other) noexcept : _observer(std::move(other._observer)) {}

 protected:
  explicit Subscriber_I(ObserverT&& observer) : _observer(std::move(observer)){};

 protected:
  ObserverT _observer;
};

}  // namespace ipcpp::publish_subscribe