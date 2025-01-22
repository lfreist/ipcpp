/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/stl/vector.h>

#include <iostream>

struct Message {
  Message() : timestamp(carry::utils::timestamp()) {}
  Message(Message&& other) noexcept : timestamp(carry::utils::timestamp()), data(std::move(other.data)) {}

  std::int64_t timestamp;
  carry::vector<char> data;
};