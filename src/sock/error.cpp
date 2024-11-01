//
// Created by lfreist on 18/10/2024.
//

#include <ipcpp/sock/error.h>

#define AS_STRING(a) #a

namespace ipcpp::sock {

namespace domain {

std::ostream& operator<<(std::ostream& os, Error error) {
  switch (error) {
    case Error::CREATION_ERROR:
      os << AS_STRING(Error::CREATION_ERROR);
      break;
    case Error::CONNECTION_ERROR:
      os << AS_STRING(Error::CONNECTION_ERROR);
      break;
    case Error::BIND_ERROR:
      os << AS_STRING(Error::BIND_ERROR);
      break;
    case Error::LISTEN_ERROR:
      os << AS_STRING(Error::LISTEN_ERROR);
      break;
  }
  return os;
}

}

namespace udp {}

namespace tcp {}

}