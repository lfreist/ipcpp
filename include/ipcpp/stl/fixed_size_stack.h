/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/config/config.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/mutex.h>

#include <atomic>
#include <cstdint>
#include <expected>
#include <tuple>

namespace ipcpp {

/**
 * @brief FixedSizeStack is a LIFO queue (stack) with a fixed size set at construction. The memory on which data and
 * stack functionality overhead (Header) is stored, must be provided at construction making it possible to store data in
 * e.g. shared memory.
 *
 * @remark FixedSizeStack is protected using a shared-mutex-like locking. c.f. include/ipcpp/utils/mutex.h
 *
 * @remark In contrast to other datatypes provided by ipcpp, this stack does not allocate the required memory using the
 *  provided allocator. This is because this stack is intended to be used as a message stack in shared memory holding
 *  messages (those messages may allocate memory in shared memory).
 *
 * @usecase The intended use case of FixedSizeStack is to create a ipcpp::shm::MappedMemory with a heap and place a
 *  FixedSizeStack at the beginning of the mapped memory as a message stack. Messages hold by the stack can then store
 *  data allocated at the mapped memory's heap.
 *
 * @tparam T
 */
template <typename T>
class FixedSizeStack {
 private:  // private nested datatypes
  /**
   * @brief Header is stored at the beginning of the provided memory. Header stores all data necessary to use the stack.
   */
  struct Header {
    Header(std::size_t capacity, config::QueueFullPolicy queue_full_policy)
        : _capacity(capacity), _queue_full_policy(queue_full_policy) {}

    Header(Header&&) noexcept = delete;
    Header(const Header&) = delete;

    config::QueueFullPolicy _queue_full_policy;
    /// index of the stacks front (first item)
    std::size_t _head_idx = 0;
    /// queues size
    /// maximum size of stack
    const std::size_t _capacity;
    shared_mutex _shared_mutex{};
  };

 public:  // public static nested datatypes
  /// A FixedSizeStacks element consists of the data (T) and a reference counter. This allows the stack to overwrite
  ///  elements that are not hold by any process when the stack runs full.
  typedef std::tuple<T, std::atomic_size_t> Element;

 public:  // public member functions
  /**
   * @brief Creates a new instance of a FixedSizeStack. Meaning memory is overwritten by Header and data.
   *
   * @param capacity max number of elements stored by the FixedSizeStack
   * @param full_policy How to handle data overflows
   * @param memory memory to use for construction of the FixedSizeStack
   * @return
   */
  static std::expected<FixedSizeStack<T>, int> create(std::size_t capacity, config::QueueFullPolicy full_policy,
                                                      std::span<uint8_t> memory) {
    std::size_t queue_size = capacity * sizeof(Element);
    if (memory.size() < queue_size) {
      std::cout << "memory size: " << memory.size() << "\n"
                << "queue_size: " << queue_size << std::endl;
      return std::unexpected(-1);
    }

    return FixedSizeStack<T>(capacity, full_policy, memory);
  }

  /**
   * @brief Creates a view onto an existing FixedSizeStack at memory. memory.data() is reinterpreted as a
   * FixedSizeStack. Make sure that a FixedSizeStack was initialized at memory.data() before.
   *
   * @param memory
   * @return
   */
  static std::expected<FixedSizeStack<T>, int> create(std::span<uint8_t> memory) { return FixedSizeStack<T>(memory); }

  static std::size_t required_memory_size(std::size_t capacity) {
    return capacity * sizeof(Element) + sizeof(Header);
  }

 public:  // public non-static member functions
  /**
   * @brief Push an element to the stack. Used when T is trivially copy constructible.
   * @param item
   */
  void push(T& item)
    requires std::is_trivially_copy_constructible_v<T>
  {
    std::unique_lock lock(_header->_shared_mutex);
    if (_header->_head_idx > _header->_capacity) {
      return;
    }
    new (head()) std::tuple<T, std::atomic_size_t>{item, 0};
    _header->_head_idx++;
  }

  /**
   * @brief Push an element to the stack. Used when T is move constructible.
   * @param item
   */
  void push(T&& item)
    requires std::is_move_constructible_v<T>
  {
    std::unique_lock lock(_header->_shared_mutex);
    if (_header->_head_idx > _header->_capacity) {
      return;
    }
    new (head()) std::tuple<T, std::atomic_size_t>{std::move(item), 0};
    _header->_head_idx++;
  }

  /**
   * @brief remove the first element of the stack.
   */
  void pop() {
    std::unique_lock lock(_header->_shared_mutex);
    if (_header->_head_idx == 0) {
      return;
    }
    _header->_head_idx = (_header->_head_idx - 1);
  }

  /**
   * @brief Returns a reference to the first element in the stack.
   *
   * @warning Calling front() on an empty stack results in UB.
   * @return
   */
  T& front() {
    std::unique_lock lock(_header->_shared_mutex);
    Element& item = _items[_header->_head_idx - 1];
    std::get<std::atomic_size_t>(item).fetch_add(1, std::memory_order_acquire);
    return std::get<T>(item);
  }

  /**
   * @brief Returns whether the stack is empty or not
   * @return
   */
  bool empty() {
    std::unique_lock lock(_header->_shared_mutex);
    return _header->_head_idx == 0;
  }

  /**
   * @brief Returns the number of elements in the stack
   * @return
   */
  std::size_t size() {
    std::unique_lock lock(_header->_shared_mutex);
    if (_header->_head_idx > _header->_capacity) {
        return _header->_capacity;
    }
    return _header->_head_idx;
  }

  /**
   * @brief Returns the maximum capacity of the stack
   * @return
   */
  [[nodiscard]] std::size_t capacity() const { return _header->_capacity; }

 private:  // private non-static member functions
  /**
   * @brief Constructor for constructing and initializing a FixedSizeStack (resets the stack if data already holds a
   *  stack)
   * @param capacity
   * @param queue_full_policy
   * @param data
   */
  FixedSizeStack(std::size_t capacity, config::QueueFullPolicy queue_full_policy, std::span<uint8_t> data)
      : _header(reinterpret_cast<Header*>(data.data())),
        _items(reinterpret_cast<Element*>(data.data()) + sizeof(Header), capacity) {
    new (_header) Header(capacity, queue_full_policy);
  }

  /**
   * @brief Constructor for constructing a FixedSizeStack with already existent data.
   * @param data
   */
  explicit FixedSizeStack(std::span<uint8_t> data) : _header(reinterpret_cast<Header*>(data.data())) {
    _items = std::span(reinterpret_cast<Element*>(data.data()) + sizeof(Header), _header->_capacity);
  }

  /**
   * @brief unsafe access to the queues tail. Only use when the mutex is owned by the caller.
   * @return
   */
  Element* head() { return &_items[_header->_head_idx]; }

 private:  // private members
  /// Header storing all information needed to use _items as stack
  Header* _header;
  /// the items are placed right after the FixedSizeStack instance
  std::span<Element> _items;
};

}  // namespace ipcpp