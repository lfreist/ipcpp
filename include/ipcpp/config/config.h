/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::config {

enum class OutOfMemoryPolicy {
  BLOCK_PRODUCER,
  DISCARD_OLDEST,
};

enum class NotificationQueuePolicy {
  LATEST_ONLY,  // empty queue whenever a new notification is published
  FIFO,         // first in, first out
  LIFO,         // last in first out
};

enum class QueueFullPolicy {
  BLOCK,
  DISCARD_OLDEST,
  ERROR
};

}