/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>

namespace ipcpp::publish_subscribe {

struct Notification {
  std::int64_t timestamp;
  std::size_t index;
};

}