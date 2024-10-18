/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/types.h>
#include <iostream>

namespace ipcpp {

std::ostream& operator<<(std::ostream& os, AccessMode am) {
  switch (am) {
    case AccessMode::READ:
      os << AS_STRING(AccessMode::READ);
      break;
    case AccessMode::WRITE:
      os << AS_STRING(AccessMode::WRITE);
      break;
  }
  return os;
}

namespace sockets {



}

namespace notification {

std::ostream& operator<<(std::ostream& os, NotificationType nt) {
  switch (nt) {
    case NotificationType::UNINITIALIZED:
      os << AS_STRING(NotificationType::UNINITIALIZED);
      break;
    case NotificationType::REGISTRATION_SUCCESS:
      os << AS_STRING(NotificationType::REGISTRATION_SUCCESS);
      break;
    case NotificationType::REGISTRATION_FAILED:
      os << AS_STRING(NotificationType::REGISTRATION_FAILED);
      break;
    case NotificationType::PUBLISHER_DOWN:
      os << AS_STRING(NotificationType::PUBLISHER_DOWN);
      break;
    case NotificationType::REGULAR:
      os << AS_STRING(NotificationType::REGULAR);
      break;
  }
  return os;
}

}

}