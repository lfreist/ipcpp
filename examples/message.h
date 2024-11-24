/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>
#include <ipcpp/datatypes/vector.h>

struct Message {
  std::int64_t timestamp;
  ipcpp::vector<char> data;
};