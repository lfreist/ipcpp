/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/linked_list.h>

#include <cstdint>
#include <span>
#include <list>

namespace ipcpp {

/**
 * @brief ListAllocator uses a pre-allocated memory space to provide allocations of T within this memory space.
 *  It is implemented as list-allocator meaning it holds a linked list of free T's from which it returns and removes the
 *  head on allocations. On deallocations, a given T is appended to the linked list as head.
 * @tparam T
 */
template <typename T>
class ListAllocator {
 public:
  ListAllocator(void* addr, std::size_t size)
      : _addr(reinterpret_cast<std::uintptr_t>(addr)) {
    if (size < sizeof(T)) {
      return;
    }
    std::size_t num_elements = (size / sizeof(T));
    for (std::size_t i = 0; i < num_elements; ++i) {
      _free_list.push_back(reinterpret_cast<T*>(_addr) + i);
    }
  }

  T* allocate() {
    if (_free_list.empty()) {
      throw std::bad_alloc();
    }
    auto* chunk = _free_list.back();
    _free_list.pop_back();
    _used_list.push_back(chunk);
    return chunk;
  }

  void deallocate(T* t) {
    if (t == nullptr) {
      return;
    }
    _free_list.push_back(t);
    _used_list.remove(t);  // can we do better? this is quite costly...
  }

 private:
  std::uintptr_t _addr = 0;
  std::list<T*> _free_list;
  // we store the _used_list to allow reusing used chunks if no chunk is available
  std::list<T*> _used_list;
};

}  // namespace ipcpp