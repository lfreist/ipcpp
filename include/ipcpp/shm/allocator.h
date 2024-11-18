/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/linked_list.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace ipcpp::shm {

constexpr std::size_t ALIGNMENT = 16;
constexpr std::size_t TCACHE_MAX_BINS = 64;
constexpr std::size_t TCACHE_MAX_PER_BIN = 7;
constexpr std::size_t MAX_FASTBIN_SIZE = 64;
constexpr std::size_t PAGE_SIZE = 4096;

inline std::size_t align_up(std::size_t size, std::size_t alignment) { return (size + alignment - 1) & ~(alignment - 1); }

class TCache {
 public:
 private:
};

class FastBins {
 public:
 private:
};

class UnsortedBin {
 public:
 private:
};

/**
 * @brief
 * MemoryLayout: | Header | ChunkAllocator | available memory ... |
 *
 * The Header stores offsets of ChunkAllocator and the available memory
 * The ChunkAllocator is used to allocate Nodes for the linked list storing information of the dynamic allocations
 */
class SharedMemoryAllocator {
 public:
  struct Chunk {
    std::size_t size;
    bool is_free;
    //   -1: writer holds this chunk;
    //    0: chunk is available for reading and writing; writer acquiring this chunk subs 1
    // 0..n: chunk is available for reading; reader acquiring this chunk adds 1
    std::atomic_int readers;
  };

 public:
  SharedMemoryAllocator(void* addr, std::size_t size)
      : _addr(reinterpret_cast<std::uintptr_t>(addr)),
        _size(size),
        _allocated_size(0),
        _top_chunk(reinterpret_cast<Chunk*>(_addr)),
        _chunk_list(new ipcpp::utils::linked_list::Node<Chunk*>(_top_chunk)) {
    _top_chunk->size = _size - sizeof(Chunk);
    _top_chunk->is_free = true;

    std::cout << "Initialized:" << std::endl;
    print_list();
  }

  std::uintptr_t allocate(std::size_t size) {
    size = align_up(size + sizeof(Chunk), ALIGNMENT);

    Chunk* chunk = allocate_from_list(size);
    if (chunk == nullptr) {
      throw std::bad_alloc();
    }
    std::cout << "Allocated:" << std::endl;
    print_list();
    _allocated_size += size;
    return reinterpret_cast<std::uintptr_t>(chunk) + sizeof(Chunk);
  }

  void deallocate(std::uintptr_t data) {
    if (data == 0) {
      return;
    }
    auto* chunk = reinterpret_cast<Chunk*>(reinterpret_cast<std::uintptr_t>(data) - sizeof(Chunk));
    chunk->is_free = true;
    _allocated_size -= chunk->size;
    coalesce(chunk);
    std::cout << "Deallocated:" << std::endl;
    print_list();
  }

  void print_list() {
    auto* node = _chunk_list;
    int i = 0;
    while (node != nullptr) {
      std::cout << "[" << i << (node->data()->is_free ? "+" : "-") << node->data()->size << "]" << std::endl;
      node = node->next();
    }
  }

 private:
  Chunk* allocate_from_list(std::size_t size) {
    auto* node = _chunk_list;
    Chunk* chunk = nullptr;
    while (true) {
      if (node == nullptr) {
        return nullptr;
      }
      chunk = node->data();
      if (chunk->is_free) {
        if (chunk->size == size) {
          chunk->is_free = false;
          break;
        } else if (chunk->size > size) {
          auto* new_chunk = reinterpret_cast<Chunk*>(reinterpret_cast<std::uintptr_t>(chunk) + size);
          node->insert(new_chunk);
          new_chunk->size = chunk->size - size;
          new_chunk->is_free = true;
          chunk->is_free = false;
          chunk->size = size;
          break;
        }
      }
      node = node->next();
    }
    return chunk;
  }

  void coalesce(Chunk* chunk) {
    auto* node = _chunk_list;
    while (true) {
      if (node == nullptr) {
        return;
      }
      if (node->data() == chunk) {
        break;
      }
      node = node->next();
    }
    coalesce_forward(node);
    coalesce_backwards(node);
  }

  static void coalesce_backwards(ipcpp::utils::linked_list::Node<Chunk*>* node) {
    while (node->prev() != nullptr) {
      if (!node->prev()->data()->is_free) {
        break;
      }
      node = node->prev();
      auto* popped = node->next()->pop();
      node->data()->size += popped->data()->size;
      delete popped;
    }
  }

  static void coalesce_forward(ipcpp::utils::linked_list::Node<Chunk*>* node) {
    while (node->next() != nullptr) {
      if (!node->next()->data()->is_free) {
        break;
      }
      auto* popped = node->next()->pop();
      node->data()->size += popped->data()->size;
      delete popped;
    }
  }

 private:
  std::uintptr_t _addr;
  std::size_t _size;
  std::size_t _allocated_size = 0;

  Chunk* _top_chunk;
  ipcpp::utils::linked_list::Node<Chunk*>* _chunk_list;
};

template <typename T>
class SharedMemoryAllocatorAdapter {
 public:
  using value_type = T;

  explicit SharedMemoryAllocatorAdapter(SharedMemoryAllocator* allocator) : allocator_(allocator) {}

  template <typename U>
  explicit SharedMemoryAllocatorAdapter(const SharedMemoryAllocatorAdapter<U>& other) noexcept
      : allocator_(other.allocator_) {}

  T* allocate(std::size_t n) { return reinterpret_cast<T*>(allocator_->allocate(n * sizeof(T))); }

  void deallocate(T* p, std::size_t) noexcept {
    if (p) {
      allocator_->deallocate(reinterpret_cast<std::uintptr_t>(p));
    }
  }

  template <typename U>
  struct rebind {
    using other = SharedMemoryAllocatorAdapter<U>;
  };

  SharedMemoryAllocator* allocator_;
};

}