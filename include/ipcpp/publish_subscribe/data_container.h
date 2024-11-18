/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/mutex.h>

#include <atomic>

namespace ipcpp {

template <typename T>
struct DataContainer {
  explicit DataContainer(T& data, std::size_t observer_count) : data(data), observer_count(observer_count) {}
  explicit DataContainer(T&& data, std::size_t observer_count)
      : data(std::move(data)), observer_count(observer_count) {}
  T data;
  SharedMutex shared_mutex{};
  std::atomic_size_t observer_count;
};

}  // namespace ipcpp