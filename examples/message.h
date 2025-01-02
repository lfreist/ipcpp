/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/vector.h>

#include <iostream>

struct Message {
  Message() : timestamp(ipcpp::utils::timestamp()) {}
  Message(Message&& other) noexcept : timestamp(ipcpp::utils::timestamp()), data(std::move(other.data)) {}

  std::int64_t timestamp;
  ipcpp::vector<char> data;
};