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

/**
 * @brief
 *
 * offset: byte distance from _memory, used for AllocatorListNodes
 */
class DynamicAllocator {
  struct Header {
    std::ptrdiff_t list_head_offset;
    Mutex mutex;
  };

  struct AllocatorListNode {
    std::size_t size;            // size of actual free memory (without this header)
    std::ptrdiff_t next_offset;  // offset to the next AllocatorListNode
    std::ptrdiff_t prev_offset;  // offset to the previous AllocatorListNode
    bool is_free;
  };

  static bool is_free(AllocatorListNode* node) {
    return node->size > 0;
  }

  static void set_is_free(AllocatorListNode* node, bool value) {
    constexpr std::int64_t SIGN_BIT_MASK_1 = static_cast<std::int64_t>(0x8000000000000000);
    constexpr std::int64_t SIGN_BIT_MASK_0 = ~SIGN_BIT_MASK_1;
    if (value) {
      node->size |= SIGN_BIT_MASK_1;
    } else {
      node->size &= ~SIGN_BIT_MASK_0;
    }
  }

  static std::size_t size(AllocatorListNode* node) {
    if (node->size > 0) {
      return node->size;
    } else {
      return node->size * (-1);
    }
  }

  static void set_size(AllocatorListNode* node, std::uint64_t value) {
    if (node->size > 0) {
      node->size = static_cast<int64_t>(value);
    } else {
      node->size = static_cast<int64_t>(value) * (-1);
    }
  }

  static constexpr std::ptrdiff_t invalid_offset = -1;

 public:
  DynamicAllocator(void* addr, std::size_t size)
      : _header(new(addr) Header{.list_head_offset = 0}),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {
    new (_memory) AllocatorListNode{.size = size - align_up(sizeof(Header)) - align_up(sizeof(AllocatorListNode)),
                                    .next_offset = invalid_offset,
                                    .prev_offset = invalid_offset,
                                    .is_free = true};
    _header->list_head_offset = 0;
  }

  explicit DynamicAllocator(void* addr)
      : _header(reinterpret_cast<Header*>(addr)),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {}

  template <typename T>
    requires(!std::is_same_v<T, void>)
  T* allocate(std::size_t num_t = 1) {
    return reinterpret_cast<T*>(allocate(num_t * sizeof(T)));
  }

  void* allocate(std::size_t num_bytes) { return addr_from_offset(allocate_get_index(num_bytes)); }

  std::ptrdiff_t allocate_get_index(std::size_t size_bytes) { return allocate_from_list(size_bytes); }

  void deallocate(void* addr) {
    auto* node =
        reinterpret_cast<AllocatorListNode*>(reinterpret_cast<uint8_t*>(addr) - align_up(sizeof(AllocatorListNode)));
    node->is_free = true;
    merge_forward(node);
    merge_backward(node);
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
  std::ptrdiff_t allocate_from_list(std::size_t size_bytes) {
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    while (true) {
      if (node->is_free && node->size >= size_bytes) {
        // use the current node
        node->is_free = false;
        if (node->size - size_bytes > align_up(sizeof(AllocatorListNode))) {
          insert_node(node,
                      offset_from_addr(node) + static_cast<std::ptrdiff_t>(align_up(sizeof(AllocatorListNode))) +
                          static_cast<std::ptrdiff_t>(size_bytes),
                      node->size - size_bytes - align_up(sizeof(AllocatorListNode)));
        }
        node->size = size_bytes;
        return offset_from_addr(node) + static_cast<std::ptrdiff_t>(align_up(sizeof(AllocatorListNode)));
      } else {
        if (node->next_offset == 0) {
          throw std::bad_alloc();
        }
        node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
      }
    }
  }

  void* addr_from_offset(std::ptrdiff_t offset) {
    return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_memory) + offset);
  }

  std::ptrdiff_t offset_from_addr(void* addr) {
    return reinterpret_cast<uint8_t*>(addr) - reinterpret_cast<uint8_t*>(_memory);
  }

  void insert_node(AllocatorListNode* node, std::ptrdiff_t new_nodes_offset, std::size_t new_nodes_size) {
    std::ptrdiff_t next_offset = node->next_offset;
    node->next_offset = new_nodes_offset;
    new (addr_from_offset(new_nodes_offset))
        AllocatorListNode{.size = new_nodes_size, .next_offset = next_offset, .prev_offset = offset_from_addr(node), .is_free=true};
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
  Header* _header;
  void* _memory;
};

}  // namespace ipcpp::shm