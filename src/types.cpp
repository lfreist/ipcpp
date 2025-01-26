/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/types.h>
#include <iostream>

#define AS_STRING(x) #x

namespace carry {

std::ostream& operator<<(std::ostream& os, AccessMode v) {
  switch (v) {
    case AccessMode::READ:
      os << AS_STRING(AccessMode::READ);
      break;
    case AccessMode::WRITE:
      os << AS_STRING(AccessMode::WRITE);
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, InitializationState v) {
  switch (v) {
    case InitializationState::uninitialized:
      os << AS_STRING(InitializationState::uninitialized);
      break;
    case InitializationState::initialization_in_progress:
      os << AS_STRING(InitializationState::initialization_in_progress);
      break;
    case InitializationState::initialized:
      os << AS_STRING(InitializationState::initialized);
      break;
  }
  return os;
}

}