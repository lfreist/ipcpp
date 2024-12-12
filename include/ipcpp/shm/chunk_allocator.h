/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/fixed_size_stack.h>

#include <cstdint>
#include <span>

namespace ipcpp::shm {

inline std::size_t align_up(std::size_t size, std::size_t alignment = 16) {
  return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief This ChunkAllocator is implemented for full shm support as list allocator. It stores all data in contiguous
 *  memory and allows allocations within this memory of single chunks of type T.
 *
 *  Memory Layout: | StackHeader | StackData | Chunk_0, ..., Chunk_n-1 |
 *
 *  StackHeader stores a shared_mutex for safe access from different processes, the head of the stack and the stacks
 *   capacity (n), allowing full construction of a ChunkAllocator from already constructed data.
 *  StackData are the indexes (0, ..., n-1) of the Chunks
 *  Chunk_i is a series of _m_num_bytes of size sizeof(T) that is yielded by allocate()
 *
 * @tparam T
 */
template <typename T>
class ChunkAllocator {
  struct StackHeader {
    shared_mutex mutex;
    /// points to the first free slot
    std::ptrdiff_t head = 0;
    /// capacity of the stack
    std::size_t capacity = 0;
  };

 public:
  static std::size_t required_memory_size(std::size_t num_chunks) {
    return align_up(sizeof(StackHeader)) + align_up(num_chunks * sizeof(std::size_t) + (num_chunks * sizeof(T)));
  }

  // (x * 2) + (x * 3) = y - 5
  // 5x = y - 5

  /**
   * @brief Creates a ListAllocator at addr.
   *
   * @param addr
   * @param size
   * @param num_chunks
   */
  ChunkAllocator(void* addr, std::size_t size, std::size_t num_chunks)
      : _addr(reinterpret_cast<std::uintptr_t>(addr)), _stack_header(reinterpret_cast<StackHeader*>(_addr)) {
    if (size < required_memory_size(num_chunks)) {
      return;
    }
    new (_stack_header) StackHeader{.capacity = num_chunks};
    _stack_slots = std::span<std::size_t>(
        reinterpret_cast<std::size_t*>(reinterpret_cast<uint8_t*>(_addr) + align_up(sizeof(StackHeader))), num_chunks);
    _chunks_start = _addr + align_up(sizeof(StackHeader)) + align_up(num_chunks * sizeof(std::size_t));
    _slots = std::span<T>(reinterpret_cast<T*>(_chunks_start), num_chunks);
    for (std::size_t i = 0; i < num_chunks; ++i) {
      push_to_stack(i);
    }
  }

  ChunkAllocator(void* addr, std::size_t size)
      : _addr(reinterpret_cast<std::uintptr_t>(addr)), _stack_header(reinterpret_cast<StackHeader*>(_addr)) {
    std::size_t num_chunks = (size - sizeof(StackHeader)) / (sizeof(std::size_t) + sizeof(T));
    new (_stack_header) StackHeader{.capacity = num_chunks};
    _stack_slots = std::span<std::size_t>(
        reinterpret_cast<std::size_t*>(reinterpret_cast<uint8_t*>(_addr) + align_up(sizeof(StackHeader))), num_chunks);
    _chunks_start = _addr + align_up(sizeof(StackHeader)) + align_up(num_chunks * sizeof(std::size_t));
    _slots = std::span<T>(reinterpret_cast<T*>(_chunks_start), num_chunks);
    for (std::size_t i = 0; i < num_chunks; ++i) {
      push_to_stack(i);
    }
  }

  /**
   * @brief Interprets addr as ListAllocator
   *
   * @param addr
   */
  explicit ChunkAllocator(void* addr) : _addr(reinterpret_cast<std::uintptr_t>(addr)), _stack_header(reinterpret_cast<StackHeader*>(_addr)) {
    _stack_slots = std::span<std::size_t>(
        reinterpret_cast<std::size_t*>(reinterpret_cast<uint8_t*>(_addr) + align_up(sizeof(StackHeader))), _stack_header->capacity);
    _chunks_start = _addr + align_up(sizeof(StackHeader)) + align_up(_stack_header->capacity * sizeof(std::size_t));
    _slots = std::span<T>(reinterpret_cast<T*>(_chunks_start), _stack_header->capacity);
  }

  T* allocate() {
    return index_to_ptr(allocate_get_index());
  }

  std::size_t allocate_get_index() {
    std::unique_lock lock(_stack_header->mutex);
    if (_stack_header->head == 0) {
      throw std::bad_alloc();
    }
    return pop_from_stack();
  }

  void deallocate(T* t) {
    if (t == nullptr) {
      return;
    }
    std::unique_lock lock(_stack_header->mutex);
    push_to_stack(ptr_to_index(t));
  }

  void deallocate(std::size_t index) {
    std::unique_lock(_stack_header->mutex);
    push_to_stack(index);
  }

  T* index_to_ptr(std::size_t index) { return _slots.data() + index; }

  std::ptrdiff_t ptr_to_index(T* ptr) { return ptr - reinterpret_cast<T*>(_chunks_start); }

 private:
  /**
   * @brief Pushes a value to the stack.
   *
   * @warning There is no overflow check for this operation. If the stack is at full capacity, calling push_to_stack
   *  results in UB!
   *
   *  @warning This function does not take ownership of the shared mutex. Only call it, when the shared mutex is owned
   *   by the caller!
   *
   * @param value
   */
  void push_to_stack(std::size_t value) {
    _stack_slots[_stack_header->head] = value;
    _stack_header->head++;
  }

  /**
   * @brief Removes and returns the stacks head.
   *
   * @warning If the stack is empty, calling pop_from_stack() results in UB!
   *
   *  @warning This function does not take ownership of the shared mutex. Only call it, when the shared mutex is owned
   *   by the caller!
   *
   * @return
   */
  std::size_t pop_from_stack() {
    _stack_header->head--;
    return _stack_slots[_stack_header->head];
  }

 private:
  std::uintptr_t _addr = 0;
  StackHeader* _stack_header = nullptr;
  /// the stack holds the offsets of the addr of a slot to the addr of the first slot
  std::span<std::size_t> _stack_slots;
  std::span<T> _slots;
  std::uintptr_t _chunks_start = 0;
};

}  // namespace ipcpp::shm