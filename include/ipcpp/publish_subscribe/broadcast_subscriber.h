/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/publish_subscribe/subscriber.h>
#include <ipcpp/event/domain_socket_observer.h>
#include <ipcpp/event/notification.h>

namespace ipcpp::publish_subscribe {

struct Notification {
  int64_t timestamp;
  std::size_t data_offset;
};

template <typename T,
    typename ObserverT = event::DomainSocketObserver<Notification, ipcpp::event::SubscriptionResponse<int>>>
class BroadcastSubscriber : Subscriber_I<T, ObserverT> {};

}