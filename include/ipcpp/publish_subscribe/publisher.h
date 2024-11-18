/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/event/domain_socket_notifier.h>
#include <ipcpp/publish_subscribe/data_container.h>

#include <any>
#include <expected>

namespace ipcpp::publish_subscribe {

template <typename DataT, typename NotifierT>
class Publisher {
 public:
  virtual ~Publisher() = default;
  Publisher(Publisher&& other) noexcept : _notifier(std::move(other._notifier)) {}

  virtual void publish(DataT& data) = 0;
  virtual void publish(DataT&& data) = 0;

 protected:
  explicit Publisher(NotifierT&& notifier) : _notifier(std::move(notifier)) {}

  virtual void notify_observers(typename NotifierT::notifier_base::notification_type notification) = 0;

 protected:
  NotifierT _notifier;
};

}  // namespace ipcpp::publish_subscribe