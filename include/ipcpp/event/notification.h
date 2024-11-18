//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <cstdint>

#include <ipcpp/shm/notification.h>

namespace ipcpp::event {

enum class NotificationError {
  NOTIFIER_DOWN,
  OBSERVER_DOWN,
  NOTIFICATION_ERROR,
  INVALID_NOTIFICATION,
  UNKNOWN,
};

std::ostream& operator<<(std::ostream& os, NotificationError error) {
  switch (error) {
    case NotificationError::NOTIFIER_DOWN:
      os << AS_STRING(NotificationError::NOTIFIER_DOWN);
      break;
    case NotificationError::OBSERVER_DOWN:
      os << AS_STRING(NotificationError::OBSERVER_DOWN);
      break;
    case NotificationError::NOTIFICATION_ERROR:
      os << AS_STRING(NotificationError::NOTIFICATION_ERROR);
      break;
    case NotificationError::INVALID_NOTIFICATION:
      os << AS_STRING(NotificationError::INVALID_NOTIFICATION);
      break;
    case NotificationError::UNKNOWN:
      os << AS_STRING(NotificationError::UNKNOWN);
      break;
  }
  return os;
}

enum class SubscriptionInfo {
  OK,
  NOTIFIER_UNREACHABLE,
  OBSERVER_REJECTED,
  OBSERVER_NOT_SUBSCRIBED,
  OBSERVER_ALREADY_SUBSCRIBED,
  OBSERVER_KICKED,
  OBSERVER_SOCKET_SETUP_FAILED,
  UNKNOWN,
};

std::ostream& operator<<(std::ostream& os, SubscriptionInfo info) {
  switch (info) {
    case SubscriptionInfo::OK:
      os << AS_STRING(SubscriptionInfo::OK);
      break;
    case SubscriptionInfo::NOTIFIER_UNREACHABLE:
      os << AS_STRING(SubscriptionInfo::NOTIFIER_UNREACHABLE);
      break;
    case SubscriptionInfo::OBSERVER_REJECTED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_REJECTED);
      break;
    case SubscriptionInfo::OBSERVER_NOT_SUBSCRIBED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_NOT_SUBSCRIBED);
      break;
    case SubscriptionInfo::OBSERVER_ALREADY_SUBSCRIBED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_ALREADY_SUBSCRIBED);
      break;
    case SubscriptionInfo::OBSERVER_KICKED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_KICKED);
      break;
    case SubscriptionInfo::OBSERVER_SOCKET_SETUP_FAILED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_SOCKET_SETUP_FAILED);
      break;
    case SubscriptionInfo::UNKNOWN:
      os << AS_STRING(SubscriptionInfo::UNKNOWN);
      break;
  }
  return os;
}

enum class ObserverRequest {
  SUBSCRIBE,
  CANCEL_SUBSCRIPTION,
};

template <typename T>
struct SubscriptionResponse {
  typedef T data_type;
  typedef SubscriptionInfo info_type;

  info_type info;
  data_type data;
};

}