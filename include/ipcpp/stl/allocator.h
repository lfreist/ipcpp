/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <ipcpp/utils/mutex.h>
#include <ipcpp/utils/platform.h>
#include <ipcpp/topic.h>
#include <ipcpp/utils/logging.h>

#include <cassert>
#include <memory>

namespace carry {

namespace detail {
// TODO: creating the overhead data at addr does not require a template type. Thus, we can do all of this in the Base
//       class and only implement the value_type specific member functions (allocation) in the templated child class.

/**
 * @brief Base class to provide a static void* for all template types of the child allocator
 */
class allocator_factory_base {
 protected:
  static std::uintptr_t _singleton_process_addr;
};

std::uintptr_t allocator_factory_base::_singleton_process_addr = 0;

}  // namespace detail

/**
 * @brief poo_allocator can be used to dynamically allocate memory of a given size in a pre-allocated memory space
 * (pool). In order to make it fully shared memory compatible (including sharing this Allocator among different
 * processes, the following requirements are fulfilled:
 *  - internal data are all stored in the provided memory, DynamicAllocator itself is just a convenient wrapper around
 *    these data
 *  - concerning the internal data, no raw pointers but only std::ptrdiff is used as offset of data
 *  - Once DynamicAllocator was constructed using the DynamicAllocator(void*, size_type) constructor, other processes
 *    can use this DynamicAllocator by using the DynamicAllocator(void*) constructor
 *
 * offset: byte distance from _memory, used for AllocatorListNodes
 */
template <typename T_p>
class IPCPP_API pool_allocator : public detail::allocator_factory_base {
 public:
  typedef T_p value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef std::ptrdiff_t alloc_return_type;
  typedef T_p* pointer;
  typedef const T_p* const_pointer;

 private:
  struct IPCPP_API Header {
    const size_type size;
    difference_type list_head_offset;
    mutex mutex_;
  };

  struct AllocatorListNode {
    size_type size;               // size of actual free memory (without this header)
    difference_type next_offset;  // offset to the next AllocatorListNode
    difference_type prev_offset;  // offset to the previous AllocatorListNode
    bool is_free;
  };

  static constexpr difference_type invalid_offset = -1;

  static inline std::size_t align_up(std::size_t size, std::size_t alignment = 16) {
    return (size + alignment - 1) & ~(alignment - 1);
  }

 public:
  /**
   * @brief Initializes the static instance of this allocator. After initialization, get_singleton() returns a
   *  working allocator using the data at addr.
   *
   *  @warning This parameter overload overwrites existing data at addr! Only use if the data at addr are not used by
   *   any process. Use initialize_factory(void*) to use an already initialized DynamicAllocator.
   *
   * @param addr
   * @param size
   */
  static void initialize_factory(std::uintptr_t addr, size_type size) {
    static pool_allocator<uint8_t> allocator(addr, size);
    _singleton_process_addr = addr;
  }
  /**
   * @brief Initializes the static instance of this allocator. After initialization, get_singleton() returns a
   *  working allocator using the data at addr.
   *
   *  @warning This parameter overload does not initialize the data at addr to become a valid allocator. Only use if
   *   another process initialized a DynamicAllocator at addr. Use initialize_factory(void*, size_type) to initialize
   *   a DynamicAllocator at addr.
   *
   * @param addr
   */
  static void initialize_factory(std::uintptr_t addr) { _singleton_process_addr = addr; }

  static bool factory_initialized() { return _singleton_process_addr != 0; }

  /**
   * @brief builds and returns a local wrapper for DynamicAllocator at AllocatorFactoryBase::_singleton_process_addr.
   *  AllocatorFactoryBase::_singleton_process_addr is initialized by calling initialize_factory() in the current
   *  process.
   *
   *  @warning In order to use get_singleton() to retrieve a DynamicAllocator, initialize_factory() must be called
   *  from the current process.
   *
   * @return
   */
  static pool_allocator get_singleton() {
    if (_singleton_process_addr == 0) {
      throw std::runtime_error(
          "Allocator Factory not initialized. Call DynamicAllocator<T_p>::initialize_factory first.");
    }
    return pool_allocator<T_p>(_singleton_process_addr);
  }

  /**
   * @brief Construct (and initialize) a DynamicAllocator. All data needed by the allocator are stored in the provided
   *  memory starting at addr.
   *
   *  @warning This constructor overwrites data at addr. If a DynamicAllocator is already initialized at addr and its
   *   data are still valid use DynamicAllocator(void*) instead to just use the initialized allocator.
   *
   *  @warning If the provided size is smaller than the size of the allocator overhead data, the behaviour is undefined.
   *   In other words, the provided size should be at least
   *   align_up(sizeof(Header)) + align_up(sizeof(AllocatorListNode))
   *
   * @param addr
   * @param size
   */
  pool_allocator(std::uintptr_t addr, size_type size)
      : _header(new(reinterpret_cast<void*>(addr)) Header{.size = size, .list_head_offset = 0}),
        _memory(addr + align_up(sizeof(Header))) {
    new (reinterpret_cast<void*>(_memory)) AllocatorListNode{.size = size - align_up(sizeof(Header)) - align_up(sizeof(AllocatorListNode)),
                                    .next_offset = invalid_offset,
                                    .prev_offset = invalid_offset,
                                    .is_free = true};
  }

  /**
   * @brief Constructs a DynamicAllocator using the data at addr.
   *
   * @warning If no DynamicAllocator was initialized at addr, the instance is invalid and its behaviour is undefined.
   *  Use DynamicAllocator(void*, size_type) to initialize a DynamicAllocator.
   *
   * @param addr
   */
  explicit pool_allocator(std::uintptr_t addr)
      : _header(reinterpret_cast<Header*>(addr)),
        _memory(addr + align_up(sizeof(Header))) {}

  /// default trivially copyable and movable
  pool_allocator(pool_allocator&& other) noexcept = default;
  pool_allocator(const pool_allocator& other) = default;

  /**
   * @brief Allocate n value_types and get pointer to first value_type.
   *  Actually align_up(n * sizeof(value_type) bytes are allocated. E.g. when allocating a single int (4 _m_num_bytes), a chunk
   *  of 16 _m_num_bytes is allocated.
   *
   *  TODO: for small allocations as in the example (a single int), a separate bin should be initialized to prevent
   *   allocation of too many too large chunks (e.g. separate allocation of 4 ints results in 64 bytes but only 16 _m_num_bytes
   *   are needed)
   *
   * @param n
   * @return
   */
  pointer allocate(size_type n = 1) { return _m_allocate_from_list_ptr(n * sizeof(value_type)).first; }

  /**
   * @brief Allocate n value_types and return the offset of their address to _memory.
   * @param n
   * @return
   */
  difference_type allocate_offset(size_type n) {
    std::unique_lock lock(_header->mutex_);
    return _m_allocate_from_list(align_up(n * sizeof(value_type))).first;
  }

  std::pair<pointer, size_type> allocate_at_least(size_type n) {
    std::unique_lock lock(_header->mutex_);
    return _m_allocate_from_list_ptr(align_up(n * sizeof(value_type)));
  }

  std::pair<difference_type, size_type> allocate_at_least_offset(size_type n) {
    std::unique_lock lock(_header->mutex_);
    return _m_allocate_from_list(align_up(n * sizeof(value_type)));
  }

  /**
   * @brief deallocate a chunk at p. p should be a pointer to the actual allocated data and NOT to the AllocatorListNode
   *  of this chunk.
   * @param p
   * @param n
   */
  void deallocate(value_type* p, size_type n) {
    std::unique_lock lock(_header->mutex_);
    auto* node =
        reinterpret_cast<AllocatorListNode*>(reinterpret_cast<uint8_t*>(p) - align_up(sizeof(AllocatorListNode)));
    node->is_free = true;
    _m_merge_forward(node);
    _m_merge_backward(node);
  }

  /**
   * @brief Get the max size that can ever be allocated by this instance.
   *
   * @return
   */
  [[nodiscard]] size_type max_size() const noexcept {
    return (_header->size - align_up(sizeof(AllocatorListNode))) / sizeof(value_type);
  }

  // --- Allocator developer information -------------------------------------------------------------------------------
  // warning: The following functions iterate through the full linked list of the allocator to compute different metrics

  /**
   * @brief compute total memory size that was allocated (including allocator overhead)
   * @return
   */
  [[nodiscard]] size_type allocated_size() const noexcept {
    std::unique_lock lock(_header->mutex_);
    auto* node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(_header->list_head_offset));
    size_type size = 0;
    while (node != nullptr) {
      if (!node->is_free) {
        // not free: add data size + header size
        size += node->size + align_up(sizeof(AllocatorListNode));
      }
      node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
    }
    return size;
  }

  /**
   * @brief compute allocated data size (overhead not included, aka memory size allocated for the requester)
   * @return
   */
  [[nodiscard]] size_type allocated_data_size() const noexcept {
    std::unique_lock lock(_header->mutex_);
    auto* node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(_header->list_head_offset));
    size_type size = 0;
    while (node != nullptr) {
      if (!node->is_free) {
        // not free: add data size
        size += node->size;
      }
      node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
    }
    return size;
  }

  /**
   * @brief computes fragmentation as "Maximum Contiguous Free Block" metric
   *
   * fragmentation = (largest free block size)/(total free memory)
   *
   * @interpretation A lower value indicates higher fragmentation, as memory is more scattered into smaller chunks.
   *  The ideal value would be 1.0
   *
   * @return
   */
  [[nodiscard]] double fragmentation() {
    std::unique_lock lock(_header->mutex__);
    size_type largest_free_block = 0;
    size_type total_free_memory = 0;
    auto* node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(_header->list_head_offset));
    while (node != nullptr) {
      if (node->is_free) {
        // not free: add data size
        total_free_memory += node->size;
        if (node->size > largest_free_block) {
          largest_free_block = node->size;
        }
      }
      node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
    }
    return static_cast<double>(largest_free_block) / static_cast<double>(total_free_memory);
  }
  // -------------------------------------------------------------------------------------------------------------------

  /**
   * @brief Compute the actual address given an offset.
   *
   * @param offset
   * @return
   */
  [[nodiscard]] value_type* offset_to_pointer(difference_type offset) const noexcept {
    if (offset < 0 || static_cast<size_type>(offset) > _header->size) {
      return nullptr;
    }
    return reinterpret_cast<value_type*>(reinterpret_cast<uint8_t*>(_memory) + offset);
  }

  /**
   * @brief Compute an offset given an address
   * @param addr
   * @return
   */
  [[nodiscard]] difference_type pointer_to_offset(const void* addr) const noexcept {
    if (addr == nullptr || addr > (reinterpret_cast<uint8_t*>(_memory) + _header->size)) {
      return invalid_offset;
    }
    return reinterpret_cast<const uint8_t*>(addr) - reinterpret_cast<uint8_t*>(_memory);
  }

 private:
  std::pair<pointer, size_type> _m_allocate_from_list_ptr(size_type bytes) {
    assert(_header->mutex_.is_locked());
    auto* node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(_header->list_head_offset));
    while (true) {
      if (node->is_free && node->size >= bytes) {
        // use the current node
        node->is_free = false;
        if (node->size - bytes > align_up(sizeof(AllocatorListNode))) {
          _m_insert_node(node,
                         pointer_to_offset(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode))) +
                             static_cast<difference_type>(bytes),
                         node->size - bytes - align_up(sizeof(AllocatorListNode)));
          node->size = bytes;
        }
        return {reinterpret_cast<pointer>(reinterpret_cast<uint8_t*>(node) +
                                          static_cast<difference_type>(align_up(sizeof(AllocatorListNode)))),
                node->size};
      } else {
        if (node->next_offset == invalid_offset) {
          throw std::bad_alloc();
        }
        node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
      }
    }
  }

  /**
   * @brief Return the offset of the allocated chunk without AllocatorListNode
   *  | AllocatorListNode | data ... |
   *                      ^
   *                      return this _m_num_bytes index
   * @param size_bytes
   * @return
   */
  std::pair<difference_type, size_type> _m_allocate_from_list(size_type size_bytes) {
    assert(_header->mutex_.is_locked());
    auto* node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(_header->list_head_offset));
    while (true) {
      if (node->is_free && node->size >= size_bytes) {
        // use the current node
        node->is_free = false;
        if (node->size - size_bytes > align_up(sizeof(AllocatorListNode))) {
          _m_insert_node(node,
                         pointer_to_offset(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode))) +
                             static_cast<difference_type>(size_bytes),
                         node->size - size_bytes - align_up(sizeof(AllocatorListNode)));
          node->size = size_bytes;
        }
        auto data_offset = pointer_to_offset(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode)));
        return {data_offset, node->size};
      } else {
        if (node->next_offset == invalid_offset) {
          throw std::bad_alloc();
        }
        node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
      }
    }
  }

  /**
   * @brief Construct and insert a new node right after node at new_nodes_offset with size set to new_nodes_size
   * @param node
   * @param new_nodes_offset
   * @param new_nodes_size
   */
  void _m_insert_node(AllocatorListNode* node, difference_type new_nodes_offset, size_type new_nodes_size) {
    assert(_header->mutex_.is_locked());
    difference_type next_offset = node->next_offset;
    node->next_offset = new_nodes_offset;
    new (offset_to_pointer(new_nodes_offset)) AllocatorListNode{
        .size = new_nodes_size, .next_offset = next_offset, .prev_offset = pointer_to_offset(node), .is_free = true};
    if (next_offset != invalid_offset) {
      auto* next_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(next_offset));
      next_node->prev_offset = new_nodes_offset;
    }
  }

  /**
   * @brief Merge nodes starting at node using node->next_node until the next node is not free anymore. Merging means
   *  that two AllocatorListNodes become a single AllocatorListNode with the sum of the sizes of the two.
   *
   * @param node
   */
  void _m_merge_forward(AllocatorListNode* node) {
    assert(_header->mutex_.is_locked());
    if (node->next_offset == invalid_offset) {
      return;
    }
    size_type merged_size = node->size;
    auto* next_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
    while (true) {
      if (next_node == nullptr) {
        node->next_offset = -1;
        break;
      }
      if (!next_node->is_free) {
        next_node->prev_offset = pointer_to_offset(node);
        break;
      }
      merged_size += next_node->size + align_up(sizeof(AllocatorListNode));
      next_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(next_node->next_offset));
    }
    node->size = merged_size;
    node->next_offset = pointer_to_offset(next_node);
  }

  /**
   * @brief Merges two nodes starting at node using node->prev_node until the prev node is not free anymore. Merging
   *  means that two AllocatorListNodes become a single AllocatorListNode with the sum of the sizes of the two.
   *
   * TODO: instead of recursively merging two nodes, we can collect all data until a non-free node is reached and then
   *  merge them all at one into a single AllocatorListNode.
   *
   * @param node
   */
  void _m_merge_backward(AllocatorListNode* node) {
    assert(_header->mutex_.is_locked());
    if (node->prev_offset == invalid_offset) {
      return;
    }
    size_type merged_size = node->size;
    auto* prev_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->prev_offset));
    AllocatorListNode* prev_next_node = node;
    while (true) {
      if (prev_node == nullptr) {
        break;
      }
      if (!prev_node->is_free) {
        break;
      }
      merged_size += prev_node->size + align_up(sizeof(AllocatorListNode));
      prev_next_node = prev_node;
      prev_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(prev_node->prev_offset));
    }
    prev_next_node->size = merged_size;
    auto* next_node = reinterpret_cast<AllocatorListNode*>(offset_to_pointer(node->next_offset));
    if (next_node != nullptr) {
      next_node->prev_offset = pointer_to_offset(prev_next_node);
    }
    prev_next_node->next_offset = node->next_offset;
  }

 private:
  /// located in provided memory right before _memory. aligned at 16 _m_num_bytes
  Header* _header = nullptr;
  /// located in provided memory right after aligned _header
  std::uintptr_t _memory = 0;
};

}  // namespace carry