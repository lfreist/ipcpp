/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/chunk_allocator.h>

#include <memory>
#include <span>

namespace ipcpp::shm {

template <typename _Tp, template <typename _T> typename _Alloc = ChunkAllocator>
class List {
 public:
  struct ListNode;

  typedef _Tp value_type;
  typedef _Alloc<ListNode> allocator_type;

 public:
  struct ListNode {
    value_type value;
    std::size_t index_next;
    std::size_t index_prev;
  };

  struct Header {
    std::size_t header_index = 0;
    std::size_t tail_index = 0;
    SharedMutex mutex;
  };

 public:
  List(void* addr, std::size_t size)
      : _allocator(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header)), size - align_up(sizeof(Header))), _header(reinterpret_cast<Header*>(addr)) {
    new (_header) Header;
  }

  List(void* addr) : _allocator(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))), _header(reinterpret_cast<Header*>(addr)) {}

  void push_front(value_type& value)
    requires std::is_copy_constructible_v<value_type>
  {
    std::size_t node_idx = _allocator.allocate_get_index();
    ListNode* node_ptr = _allocator.index_to_ptr(node_idx);
    UniqueLock lock(_header->mutex);
    new (node_ptr) ListNode{.value = value, .index_next = 0, .index_prev = _header->header_index};
    _header->header_index = node_idx;
  }

  void push_front(value_type&& value)
    requires std::is_move_constructible_v<value_type>
  {
    std::size_t node_idx = _allocator.allocate_get_index();
    ListNode* node_ptr = _allocator.index_to_ptr(node_idx);
    UniqueLock lock(_header->mutex);
    new (node_ptr) ListNode{.value = std::move(value), .index_next = 0, .index_prev = _header->header_index};
    _header->header_index = node_idx;
  }

  value_type pop_front() {
    UniqueLock lock(_header->mutex);
    ListNode* node = _allocator.index_to_ptr(_header->header_index);
    std::size_t prev = node->index_prev;
    value_type value = std::move(node->value);
    _allocator.deallocate(_header->header_index);
    _header->header_index = prev;
    return value;
  }

  void push_back(value_type& value)
    requires std::is_copy_constructible_v<value_type>
  {
    std::size_t node_idx = _allocator.allocate_get_index();
    ListNode* node_ptr = _allocator.index_to_ptr(node_idx);
    UniqueLock lock(_header->mutex);
    new (node_ptr) ListNode{.value = value, .index_next = _header->tail_index, .index_prev = 0};
    _header->tail_index = node_idx;
  }

  void push_back(value_type&& value)
    requires std::is_move_constructible_v<value_type>
  {
    std::size_t node_idx = _allocator.allocate_get_index();
    ListNode* node_ptr = _allocator.index_to_ptr(node_idx);
    UniqueLock lock(_header->mutex);
    new (node_ptr) ListNode{.value = std::move(value), .index_next = _header->tail_index, .index_prev = 0};
    _header->tail_index = node_idx;
  }

  value_type pop_back() {
    UniqueLock lock(_header->mutex);
    ListNode* node = _allocator.index_to_ptr(_header->tail_index);
    std::size_t next = node->index_next;
    value_type value = std::move(node->value);
    _allocator.deallocate(_header->tail_index);
    _header->tail_index = next;
    return value;
  }

  static std::size_t required_size(std::size_t num_nodes) {
    return align_up(sizeof(Header)) + allocator_type::required_memory_size(num_nodes);
  }

 private:
  allocator_type _allocator;
  Header* _header;
};

}  // namespace ipcpp::shm