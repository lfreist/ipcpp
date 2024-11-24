/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/datatypes/list.h>
#include <ipcpp/utils/mutex.h>

namespace ipcpp::shm {

class AllocatorFactoryBase {
 protected:
  static void* _singleton_process_addr;
};

void* AllocatorFactoryBase::_singleton_process_addr = nullptr;

/**
 * @brief DynamicAllocator can be used to dynamically allocate memory of a given size in a pre-allocated memory space.
 *  In order to make it fully shared memory compatible (including sharing this Allocator among different processes, the
 *  following requirements are fulfilled:
 *  - internal data are all stored in the provided memory, DynamicAllocator itself is just a convenient wrapper around
 *    these data
 *  - concerning the internal data, no raw pointers but only std::ptrdiff is used as offset of data
 *  - Once DynamicAllocator was constructed using the DynamicAllocator(void*, size_type) constructor, other processes
 *    can use this DynamicAllocator by using the DynamicAllocator(void*) constructor
 *
 *  @warning NOT YET THREAD- NOR INTERPROCESS-SAFE
 *
 * offset: byte distance from _memory, used for AllocatorListNodes
 */
template <typename T_p>
class DynamicAllocator : public AllocatorFactoryBase {
 public:
  typedef T_p value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

 private:
  struct Header {
    size_type size;
    size_type allocated_size = 0;
    difference_type list_head_offset;
    Mutex mutex;
  };

  struct AllocatorListNode {
    size_type size;               // size of actual free memory (without this header)
    difference_type next_offset;  // offset to the next AllocatorListNode
    difference_type prev_offset;  // offset to the previous AllocatorListNode
    bool is_free;
  };

  static constexpr difference_type invalid_offset = -1;

 public:
  static void initialize_factory(void* addr, size_type size) {
    static DynamicAllocator<uint8_t> allocator(addr, size);
    AllocatorFactoryBase::_singleton_process_addr = addr;
  }
  static void initialize_factory(void* addr) {
    AllocatorFactoryBase::_singleton_process_addr = addr;
  }

  static DynamicAllocator get_from_factory() {
    if (AllocatorFactoryBase::_singleton_process_addr == nullptr) {
      throw std::runtime_error(
          "Allocator Factory not initialized. Call DynamicAllocator<T_p>::initialize_factory first.");
    }
    return DynamicAllocator<T_p>(AllocatorFactoryBase::_singleton_process_addr);
  }

  DynamicAllocator(void* addr, size_type size)
      : _header(new(addr) Header{.size = size, .allocated_size = 0, .list_head_offset = 0}),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {
    new (_memory) AllocatorListNode{.size = size - align_up(sizeof(Header)) - align_up(sizeof(AllocatorListNode)),
                                    .next_offset = invalid_offset,
                                    .prev_offset = invalid_offset,
                                    .is_free = true};
  }

  explicit DynamicAllocator(void* addr)
      : _header(reinterpret_cast<Header*>(addr)),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {}

  DynamicAllocator(DynamicAllocator&& other) noexcept {
    std::swap(_header, other._header);
    std::swap(_memory, other._memory);
  }

  DynamicAllocator(const DynamicAllocator& other) : _header(other._header), _memory(other._memory) {}

  template <typename T>
    requires(!std::is_same_v<T, void>)
  T* allocate(size_type n = 1) {
    return reinterpret_cast<T*>(addr_from_offset(allocate_get_index(n)));
  }

  difference_type allocate_get_index(size_type n) {
    difference_type data_offset = allocate_from_list(align_up(n * sizeof(value_type)));
    return data_offset;
  }

  void deallocate(value_type* p, size_type n) {
    auto* node =
        reinterpret_cast<AllocatorListNode*>(reinterpret_cast<uint8_t*>(p) - align_up(sizeof(AllocatorListNode)));
    _header->allocated_size -= node->size + align_up(sizeof(AllocatorListNode));
    node->is_free = true;
    merge_forward(node);
    merge_backward(node);
  }

  [[nodiscard]] size_type max_size() const noexcept { return _header->size; }

  [[nodiscard]] size_type allocated_size() const noexcept { return _header->allocated_size; }

  [[nodiscard]] value_type* addr_from_offset(difference_type offset) const noexcept {
    if (offset < 0) {
      return nullptr;
    }
    return reinterpret_cast<value_type*>(reinterpret_cast<uint8_t*>(_memory) + offset);
  }

  [[nodiscard]] difference_type offset_from_addr(void* addr) const noexcept {
    if (addr == nullptr) {
      return -1;
    }
    return reinterpret_cast<uint8_t*>(addr) - reinterpret_cast<uint8_t*>(_memory);
  }

 private:
  /**
   * @brief Return the offset of the allocated chunk without AllocatorListNode
   *  | AllocatorListNode | data ... |
   *                      ^
   *                      return this bytes index
   * @param size_bytes
   * @return
   */
  difference_type allocate_from_list(size_type size_bytes) {
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    while (true) {
      if (node->is_free && node->size >= size_bytes) {
        // use the current node
        node->is_free = false;
        if (node->size - size_bytes > align_up(sizeof(AllocatorListNode))) {
          insert_node(node,
                      offset_from_addr(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode))) +
                          static_cast<difference_type>(size_bytes),
                      node->size - size_bytes - align_up(sizeof(AllocatorListNode)));
        }
        node->size = size_bytes;
        _header->allocated_size += size_bytes + align_up(sizeof(AllocatorListNode));
        auto data_offset = offset_from_addr(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode)));
        return data_offset;
      } else {
        if (node->next_offset == 0) {
          throw std::bad_alloc();
        }
        node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
      }
    }
  }

  void insert_node(AllocatorListNode* node, difference_type new_nodes_offset, size_type new_nodes_size) {
    difference_type next_offset = node->next_offset;
    node->next_offset = new_nodes_offset;
    new (addr_from_offset(new_nodes_offset)) AllocatorListNode{
        .size = new_nodes_size, .next_offset = next_offset, .prev_offset = offset_from_addr(node), .is_free = true};
    if (next_offset != invalid_offset) {
      auto* next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(next_offset));
      next_node->prev_offset = new_nodes_offset;
    }
  }

  void merge_forward(AllocatorListNode* node) {
    if (node->next_offset == invalid_offset) {
      return;
    }
    auto* next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
    if (!next_node->is_free) {
      return;
    }
    if (next_node->next_offset != invalid_offset) {
      auto* next_next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(next_node->next_offset));
      next_next_node->prev_offset = offset_from_addr(node);
    }
    node->next_offset = next_node->next_offset;
    node->size += next_node->size + align_up(sizeof(AllocatorListNode));
    merge_forward(next_node);
  }

  void merge_backward(AllocatorListNode* node) {
    if (node->prev_offset == invalid_offset) {
      return;
    }
    auto* prev_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->prev_offset));
    if (!prev_node->is_free) {
      return;
    }
    if (prev_node->prev_offset != invalid_offset) {
      auto* prev_prev_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(prev_node->prev_offset));
      prev_prev_node->next_offset = offset_from_addr(node);
    }
    node->prev_offset = prev_node->prev_offset;
    prev_node->size += node->size + align_up(sizeof(AllocatorListNode));
    merge_backward(prev_node);
  }

 private:
  /// located in provided memory right before _memory. aligned at 16 bytes
  Header* _header = nullptr;
  /// located in provided memory right after aligned _header
  void* _memory = nullptr;
};

}  // namespace ipcpp::shm