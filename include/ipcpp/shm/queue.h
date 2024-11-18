/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/config/config.h>
#include <ipcpp/shm/data_view.h>
#include <ipcpp/shm/error.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/types.h>
#include <ipcpp/utils/mutex.h>

#include <cstdint>
#include <expected>
#include <atomic>

namespace ipcpp::shm {

template <typename T>
struct QueueItem {
  struct Header {
    /// count the number of unique reads (each unique reader adds a single unique read on its first read only)
    uint64_t num_unique_reads = 0;
  };

  Header header;
  T value;
};

/**
 * @brief
 * @tparam T
 */
template <typename T>
class FixedSizeQueue {
 private:
  struct Header {
    std::size_t head_idx{};
    std::size_t tail_idx{};
    std::size_t size{};
    const std::size_t capacity{};
    utils::SharedMutex shared_mutex;
  };

 public:
  static std::expected<FixedSizeQueue<T>, int> create(std::size_t capacity, config::QueueFullPolicy full_policy,
                                                std::span<uint8_t> memory) {
    std::size_t queue_size = capacity * sizeof(QueueItem<T>) + sizeof(Header);
    if (memory.size() < queue_size) {
      return std::unexpected(-1);
    }

    FixedSizeQueue<T> self(capacity, full_policy, memory);
  }

 public:
  QueueItem<T>* push(T&& item) {
    utils::UniqueLock lock(_header->shared_mutex);
    if (_header->size >= _header->capacity) {
      // TODO: add behaviour for QueueFullPolicy
      return nullptr;
    }
    *tail() = QueueItem<T>(std::move(item));
    _header->tail_idx = (_header->tail_idx + 1) % _header->capacity;
    _header->size++;
  }

  void pop() {
    utils::UniqueLock lock(_header->shared_mutex);
    _header->head_idx = (_header->head_idx + 1) & _header->capacity;
    _header->size--;
  }

  QueueItem<T>& front() {
    utils::SharedLock lock(_header->shared_mutex);
    return _items[_header->head_idx];
  }

  bool empty() {
    utils::SharedLock lock(_header->shared_mutex);
    return _header->size == 0;
  }

  std::size_t size() {
    utils::SharedLock lock(_header->shared_mutex);
    return _header->size;
  }

  [[nodiscard]] std::size_t capacity() const { return _header->capacity; }

 private:
  FixedSizeQueue(std::size_t capacity, config::QueueFullPolicy queue_full_policy, std::span<uint8_t> data)
      : _queue_full_policy(queue_full_policy) {
    _header = static_cast<Header*>(data.data());
    _items = std::span<QueueItem<T>>(data.data() + sizeof(Header), capacity);
    *_header = Header {.size=0, .tail_idx=0, .head_idx=0, .capacity=capacity};
  }

  QueueItem<T>* tail() {
    utils::SharedLock lock(_header->shared_mutex);
    return _items[_header->tail_idx];
  }

  QueueItem<T>* head() {
    utils::SharedLock lock(_header->shared_mutex);
    return _items[_header->head_idx.load()];
  }

 private:
  config::QueueFullPolicy _queue_full_policy;
  Header* _header = nullptr;
  std::span<uint8_t> _items;
};

}  // namespace ipcpp::shm