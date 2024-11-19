/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>

namespace ipcpp::publish_subscribe {

struct SubscriptionResponseData {
  /// shm list size (used by ChunkAllocator)
  std::size_t list_size;
  /// shm heap size (used by DynamicAllocator)
  std::size_t heap_size;
};

}