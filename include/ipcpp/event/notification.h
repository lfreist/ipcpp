//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <iostream>

#define AS_STRING(x) #x

namespace ipcpp::event {

enum class NotificationError {
  NOTIFIER_DOWN,
  OBSERVER_DOWN,
  NOTIFICATION_ERROR,
  INVALID_NOTIFICATION,
  TIMEOUT,
  TIMEOUT_SETUP_ERROR,
  NOT_SUBSCRIBED,
  SUBSCRIPTION_PAUSED,
  UNKNOWN,
};

inline std::ostream& operator<<(std::ostream& os, const NotificationError error) {
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
    case NotificationError::TIMEOUT:
      os << AS_STRING(NotificationError::TIMEOUT);
      break;
    case NotificationError::TIMEOUT_SETUP_ERROR:
      os << AS_STRING(NotificationError::TIMEOUT_SETUP_ERROR);
      break;
    case NotificationError::UNKNOWN:
      os << AS_STRING(NotificationError::UNKNOWN);
      break;
    case NotificationError::NOT_SUBSCRIBED:
      os << AS_STRING(NotificationError::NOT_SUBSCRIBED);
      break;
    case NotificationError::SUBSCRIPTION_PAUSED:
      os << AS_STRING(NotificationError::SUBSCRIPTION_PAUSED);
      break;
    default:
      break;
  }
  return os;
}

enum class SubscriptionError {
  NO_ERROR,
  NOTIFIER_UNREACHABLE,
  OBSERVER_REJECTED,
  OBSERVER_NOT_SUBSCRIBED,
  OBSERVER_ALREADY_SUBSCRIBED,
  OBSERVER_KICKED,
  OBSERVER_SETUP_FAILED,
  UNKNOWN,
};

inline std::ostream& operator<<(std::ostream& os, const SubscriptionError info) {
  switch (info) {
    case SubscriptionError::NO_ERROR:
      os << AS_STRING(SubscriptionInfo::NO_ERROR);
    break;
    case SubscriptionError::NOTIFIER_UNREACHABLE:
      os << AS_STRING(SubscriptionInfo::NOTIFIER_UNREACHABLE);
    break;
    case SubscriptionError::OBSERVER_REJECTED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_REJECTED);
      break;
    case SubscriptionError::OBSERVER_NOT_SUBSCRIBED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_NOT_SUBSCRIBED);
      break;
    case SubscriptionError::OBSERVER_ALREADY_SUBSCRIBED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_ALREADY_SUBSCRIBED);
      break;
    case SubscriptionError::OBSERVER_KICKED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_KICKED);
      break;
    case SubscriptionError::OBSERVER_SETUP_FAILED:
      os << AS_STRING(SubscriptionInfo::OBSERVER_SETUP_FAILED);
      break;
    case SubscriptionError::UNKNOWN:
      os << AS_STRING(SubscriptionInfo::UNKNOWN);
      break;
  }
  return os;
}

enum class ObserverRequest {
  SUBSCRIBE,
  CANCEL_SUBSCRIPTION,
  PAUSE_SUBSCRIPTION,
  RESUME_SUBSCRIPTION,
};

template <typename T>
struct SubscriptionResponse {
  typedef T data_type;
  typedef SubscriptionError info_type;

  info_type info;
  data_type data;
};

}  // namespace ipcpp::event