/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::event {

enum class factory_error {
  SOCKET_CREATION_FAILED,
  SOCKET_CONNECT_FAILED,
  SOCKET_BIND_FAILED,
  SOCKET_LISTEN_FAILED,

  SHM_CREATE_FAILED,
};

}