//
// Created by lfreist on 18/10/2024.
//

#pragma once
#include <ostream>

namespace ipcpp::sock {

namespace domain {

enum class Error {
  CREATION_ERROR,    // socket(...) failed
  CONNECTION_ERROR,  // connect(...) failed
  BIND_ERROR,
  LISTEN_ERROR,
};

std::ostream& operator<<(std::ostream& os, Error am);

}

}