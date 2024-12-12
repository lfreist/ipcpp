/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/vector.h>

#include <iostream>

enum class MessageType {
  EXIT,
  ERROR,
  REGULAR
};

struct Message {
  std::int64_t timestamp;
  MessageType message_type;
  ipcpp::vector<char> data;
  ~Message() {
    std::cout << " -> Destroying message " << std::string_view(data.data(), data.size()) << std::endl;
  }
};