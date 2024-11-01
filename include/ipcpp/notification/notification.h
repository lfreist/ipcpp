//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <cstdint>

#include <ipcpp/shm/notification.h>

namespace ipcpp::notification {

enum class NotificationError {
  SENDER_DOWN,
  RECEIVER_DOWN,
  PUBLISH_ERROR,
  INVALID_NOTIFICATION,
  RECEIVER_REJECTED,
  UNKNOWN,
};

std::ostream& operator<<(std::ostream& os, NotificationError error) {
  switch (error) {
    case NotificationError::SENDER_DOWN:
      os << AS_STRING(NotificationError::SENDER_DOWN);
      break;
    case NotificationError::RECEIVER_DOWN:
      os << AS_STRING(NotificationError::RECEIVER_DOWN);
      break;
    case NotificationError::PUBLISH_ERROR:
      os << AS_STRING(NotificationError::PUBLISH_ERROR);
      break;
    case NotificationError::INVALID_NOTIFICATION:
      os << AS_STRING(NotificationError::INVALID_NOTIFICATION);
      break;
    case NotificationError::RECEIVER_REJECTED:
      os << AS_STRING(NotificationError::RECEIVER_REJECTED);
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
    case SubscriptionInfo::UNKNOWN:
      os << AS_STRING(SubscriptionInfo::UNKNOWN);
      break;
  }
  return os;
}

enum class SubscriptionRequest {
  SUBSCRIBE,
  UNSUBSCRIBE,
};

template <typename T>
struct SubscriptionResponse {
  typedef T data_type;
  typedef SubscriptionInfo info_type;

  info_type info;
  data_type data;
};

}