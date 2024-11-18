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

#include <atomic>
#include <cstdint>
#include <expected>
#include <tuple>

namespace ipcpp {

/**
 * @brief FixedSizeQueue is a queue with a fixed size set at construction. The memory on which data and queue
 *  functionality overhead (Header) is stored, must be provided at construction making it possible to store data in e.g.
 *  shared memory. Functionality is similar to std::queue but additionally allows popping and accessing elements from
 *  both ends (head and tail).
 *
 * @remark FixedSizeQueue is protected using a shared-mutex-like locking. c.f. include/ipcpp/utils/mutex.h making it
 *  thread- and interprocess-safe.
 *
 * @remark In contrast to other datatypes provided by ipcpp, this queue does not allocate the required memory using the
 *  provided allocator. This is because this queue is intended to be used as a message queue in shared memory holding
 *  messages (those messages may allocate memory in shared memory).
 *
 * @usecase The intended use case of FixedSizeQueue is to create a ipcpp::shm::MappedMemory with a heap and place a
 *  FixedSizeQueue at the beginning of the mapped memory as a message queue. Messages hold by the queue can then store
 *  data allocated at the mapped memory's heap.
 *
 * @tparam T
 */
template <typename T>
class FixedSizeQueue {
 private:  // private nested datatypes
  /**
   * @brief Header is stored at the beginning of the provided memory. Header stores all data necessary to use the queue.
   */
  struct Header {
    Header(std::size_t capacity) : _capacity(capacity) {}

    Header(Header&&) noexcept = delete;
    Header(const Header&) = delete;

    /// indicates whether the queue is at max capacity. needed because head_idx and tail_idx are equal when the queue is
    /// empty and when the queue is at max capacity
    bool _is_full = false;
    /// index of the queues front (first item)
    std::size_t _head_idx = 0;
    /// index of the first free slot
    std::size_t _tail_idx = 0;
    /// queues size
    /// maximum size of queue
    const std::size_t _capacity;
    SharedMutex _shared_mutex{};
  };

 public:  // public static nested datatypes
  /// A FixedSizeQueues element consists of the data (T) and a reference counter. This allows the queue to overwrite
  ///  elements that are not hold by any process when the queue runs full.
  typedef std::tuple<T, std::atomic_size_t> Element;

 public:  // public member functions
  /**
   * @brief Creates a new instance of a FixedSizeQueue. Meaning memory is overwritten by Header and data.
   *
   * @param capacity max number of elements stored by the FixedSizeQueue
   * @param full_policy How to handle data overflows
   * @param memory memory to use for construction of the FixedSizeQueue
   * @return FixedSizeQueue instance or std::unexpected(memory shortage in bytes)
   */
  static std::expected<FixedSizeQueue<T>, std::size_t> create(std::size_t capacity, std::span<uint8_t> memory) {
    if (memory.size() < required_memory_size(capacity)) {
      return std::unexpected(required_memory_size(capacity) - memory.size());
    }

    return FixedSizeQueue<T>(capacity, memory);
  }

  /**
   * @brief Creates a view onto an existing FixedSizeQueue at memory. memory.data() is reinterpreted as a
   * FixedSizeQueue. Make sure that a FixedSizeQueue was initialized at memory.data() before.
   *
   * @param memory
   * @return
   */
  static std::expected<FixedSizeQueue<T>, int> create(std::span<uint8_t> memory) { return FixedSizeQueue<T>(memory); }

  static std::size_t required_memory_size(std::size_t capacity) {
    return capacity * sizeof(Element) + sizeof(Header);
  }

 public:  // public non-static member functions
  /**
   * @brief Push an element to the queue. Used when T is trivially copy constructible.
   * @param item
   */
  void push(T& item)
    requires std::is_trivially_copy_constructible_v<T>
  {
    UniqueLock lock(_header->_shared_mutex);
    if (_header->_is_full) {
      return;
    }
    new (tail()) std::tuple<T, std::atomic_size_t>{item, 0};
    _header->_tail_idx = (_header->_tail_idx + 1) % _header->_capacity;
    if (_header->_tail_idx == _header->_head_idx) {
      _header->_is_full = true;
    }
  }

  /**
   * @brief Push an element to the queue. Used when T is move constructible.
   * @param item
   */
  void push(T&& item)
    requires std::is_move_constructible_v<T>
  {
    UniqueLock lock(_header->_shared_mutex);
    if (_header->_is_full) {
      return;
    }
    new (tail()) std::tuple<T, std::atomic_size_t>{std::move(item), 0};
    _header->_tail_idx = (_header->_tail_idx + 1) % _header->_capacity;
    if (_header->_tail_idx == _header->_head_idx) {
      _header->_is_full = true;
    }
  }

  /**
   * @brief Removes the first element of the queue (FIFO)
   */
  void pop_front() {
    UniqueLock lock(_header->_shared_mutex);
    _header->_head_idx = (_header->_head_idx + 1) % _header->_capacity;
    _header->_is_full = false;
  }

  /**
   * @brief Removes the last element of the queue (LIFO)
   */
  void pop_back() {
    UniqueLock lock(_header->_shared_mutex);
    if (_header->_tail_idx == 0) {
      _header->_tail_idx = _header->_capacity - 1;
    } else {
      _header->_tail_idx--;
    }
    _header->_is_full = false;
  }

  /**
   * @brief Returns a reference to the first element in the queue (FIFO)
   *
   * @warning Calling front() on an empty queue is UB
   * @return
   */
  T& front() {
    SharedLock lock(_header->_shared_mutex);
    Element& item = _items[_header->_head_idx];
    std::get<std::atomic_size_t>(item).fetch_add(1, std::memory_order_acquire);
    return std::get<T>(item);
  }

  /**
   * @brief Returns a reference to the last element in the queue (LIFO)
   *
   * @warning Calling back() on an emtpy queue is UB
   * @return
   */
  T& back() {
    SharedLock lock(_header->_shared_mutex);
    std::size_t tail_idx = _header->_tail_idx == 0 ? (_header->_capacity - 1) : (_header->_tail_idx - 1);
    Element& item = _items[tail_idx];
    std::get<std::atomic_size_t>(item).fetch_add(1, std::memory_order_acquire);
    return std::get<T>(item);
  }

  /**
   * @brief Returns whether the queue is empty or not
   * @return
   */
  bool empty() {
    SharedLock lock(_header->_shared_mutex);
    return _header->_head_idx == _header->_tail_idx && !_header->_is_full;
  }

  /**
   * @brief Returns the number of elements in the queue
   * @return
   */
  std::size_t size() {
    SharedLock lock(_header->_shared_mutex);
    if (_header->_tail_idx == _header->_head_idx) {
      if (_header->_is_full) {
        return _header->_capacity;
      }
      return 0;
    }
    if (_header->_tail_idx >= _header->_head_idx) {
      return _header->_tail_idx - _header->_head_idx;
    }
    return (_header->_tail_idx + capacity()) - _header->_head_idx;
  }

  /**
   * @brief Returns the maximum capacity of the queue
   * @return
   */
  [[nodiscard]] std::size_t capacity() const { return _header->_capacity; }

  bool is_full() {
    return capacity() == size();
  }

 private:  // private non-static member functions
  /**
   * @brief Constructor for constructing and initializing a FixedSizeQueue (resets the queue if data already holds a
   *  queue)
   * @param capacity
   * @param queue_full_policy
   * @param data
   */
  FixedSizeQueue(std::size_t capacity, std::span<uint8_t> data)
      : _header(reinterpret_cast<Header*>(data.data())),
        _items(reinterpret_cast<Element*>(data.data()) + sizeof(Header), capacity) {
    new (_header) Header(capacity);
  }

  /**
   * @brief Constructor for constructing a FixedSizeQueue with already existent data.
   * @param data
   */
  explicit FixedSizeQueue(std::span<uint8_t> data) : _header(reinterpret_cast<Header*>(data.data())) {
    _items = std::span(reinterpret_cast<Element*>(data.data()) + sizeof(Header), _header->_capacity);
  }

  /**
   * @brief unsafe access to the queues tail. Only use when the mutex is owned by the caller.
   * @return
   */
  Element* tail() { return &_items[_header->_tail_idx]; }

 private:  // private members
  /// Header storing all information needed to use _items as queue
  Header* _header;
  /// the items are placed right after the FixedSizeQueue instance
  std::span<Element> _items;
};

}  // namespace ipcpp