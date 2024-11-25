/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/mutex.h>

#include <cassert>

namespace ipcpp::shm {

// TODO: creating the overhead data at addr does not require a template type. Thus, we can do all of this in the Base
//       class and only implement the value_type specific member functions (allocation) in the templated child class.

/**
 * @brief Base class to provide a static void* for all template types of the child allocator
 */
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
    const size_type size;
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

  static inline std::size_t align_up(std::size_t size, std::size_t alignment = 16) {
    return (size + alignment - 1) & ~(alignment - 1);
  }

 public:
  struct memory {
    difference_type offset;
    size_type size_bytes;
  };
  /**
   * @brief Initializes the static instance of this allocator. After initialization, get_from_factory() returns a
   *  working allocator using the data at addr.
   *
   *  @warning This parameter overload overwrites existing data at addr! Only use if the data at addr are not used by
   *   any process. Use initialize_factory(void*) to use an already initialized DynamicAllocator.
   *
   * @param addr
   * @param size
   */
  static void initialize_factory(void* addr, size_type size) {
    static DynamicAllocator<uint8_t> allocator(addr, size);
    AllocatorFactoryBase::_singleton_process_addr = addr;
  }
  /**
   * @brief Initializes the static instance of this allocator. After initialization, get_from_factory() returns a
   *  working allocator using the data at addr.
   *
   *  @warning This parameter overload does not initialize the data at addr to become a valid allocator. Only use if
   *   another process initialized a DynamicAllocator at addr. Use initialize_factory(void*, size_type) to initialize
   *   a DynamicAllocator at addr.
   *
   * @param addr
   */
  static void initialize_factory(void* addr) { AllocatorFactoryBase::_singleton_process_addr = addr; }

  /**
   * @brief builds and returns a local wrapper for DynamicAllocator at AllocatorFactoryBase::_singleton_process_addr.
   *  AllocatorFactoryBase::_singleton_process_addr is initialized by calling initialize_factory() in the current
   *  process.
   *
   *  @warning In order to use get_from_factory() to retrieve a DynamicAllocator, initialize_factory() must be called
   *  from the current process.
   *
   * @return
   */
  static DynamicAllocator get_from_factory() {
    if (AllocatorFactoryBase::_singleton_process_addr == nullptr) {
      throw std::runtime_error(
          "Allocator Factory not initialized. Call DynamicAllocator<T_p>::initialize_factory first.");
    }
    return DynamicAllocator<T_p>(AllocatorFactoryBase::_singleton_process_addr);
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
  DynamicAllocator(void* addr, size_type size)
      : _header(new(addr) Header{.size = size, .list_head_offset = 0}),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {
    new (_memory) AllocatorListNode{.size = size - align_up(sizeof(Header)) - align_up(sizeof(AllocatorListNode)),
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
  explicit DynamicAllocator(void* addr)
      : _header(reinterpret_cast<Header*>(addr)),
        _memory(reinterpret_cast<uint8_t*>(addr) + align_up(sizeof(Header))) {}

  /// default trivially copyable and movable
  DynamicAllocator(DynamicAllocator&& other) noexcept = default;
  DynamicAllocator(const DynamicAllocator& other) = default;

  /**
   * @brief Allocate n value_types and get pointer to first value_type.
   *  Actually align_up(n * sizeof(value_type) bytes are allocated. E.g. when allocating a single int (4 bytes), a chunk
   *  of 16 bytes is allocated.
   *
   *  TODO: for small allocations as in the example (a single int), a separate bin should be initialized to prevent
   *   allocation of too many too large chunks (e.g. separate allocation of 4 ints results in 64 bytes but only 16 bytes
   *   are needed)
   *
   * @param n
   * @return
   */
  value_type* allocate(size_type n = 1) {
    return reinterpret_cast<value_type*>(addr_from_offset(allocate_get_index(n)));
  }

  /**
   * @brief Allocate n value_types and return the offset of their address to _memory.
   * @param n
   * @return
   */
  memory allocate_get_index(size_type n) {
    UniqueLock lock(_header->mutex);
    memory memory = _m_allocate_from_list(align_up(n * sizeof(value_type)));
    return memory;
  }

  /**
   * @brief deallocate a chunk at p. p should be a pointer to the actual allocated data and NOT to the AllocatorListNode
   *  of this chunk.
   * @param p
   * @param n
   */
  void deallocate(value_type* p, size_type n) {
    UniqueLock lock(_header->mutex);
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
  [[nodiscard]] size_type max_size() const noexcept { return _header->size - align_up(sizeof(AllocatorListNode)); }

  // --- Allocator developer information -------------------------------------------------------------------------------
  // warning: The following functions iterate through the full linked list of the allocator to compute different metrics

  /**
   * @brief compute total memory size that was allocated (including allocator overhead)
   * @return
   */
  [[nodiscard]] size_type allocated_size() const noexcept {
    UniqueLock lock(_header->mutex);
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    size_type size = 0;
    while (node != nullptr) {
      if (!node->is_free) {
        // not free: add data size + header size
        size += node->size + align_up(sizeof(AllocatorListNode));
      }
      node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
    }
    return size;
  }

  /**
   * @brief compute allocated data size (overhead not included, aka memory size allocated for the requester)
   * @return
   */
  [[nodiscard]] size_type allocated_data_size() const noexcept {
    UniqueLock lock(_header->mutex);
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    size_type size = 0;
    while (node != nullptr) {
      if (!node->is_free) {
        // not free: add data size
        size += node->size;
      }
      node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
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
    UniqueLock lock(_header->mutex);
    size_type largest_free_block = 0;
    size_type total_free_memory = 0;
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    while (node != nullptr) {
      if (node->is_free) {
        // not free: add data size
        total_free_memory += node->size;
        if (node->size > largest_free_block) {
          largest_free_block = node->size;
        }
      }
      node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
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
  [[nodiscard]] value_type* addr_from_offset(difference_type offset) const noexcept {
    if (offset < 0 || offset > _header->size) {
      return nullptr;
    }
    return reinterpret_cast<value_type*>(reinterpret_cast<uint8_t*>(_memory) + offset);
  }

  /**
   * @brief Compute an offset given an address
   * @param addr
   * @return
   */
  [[nodiscard]] difference_type offset_from_addr(void* addr) const noexcept {
    if (addr == nullptr || addr > (reinterpret_cast<uint8_t*>(_memory) + _header->size)) {
      return invalid_offset;
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
  memory _m_allocate_from_list(size_type size_bytes) {
    assert(_header->mutex.is_locked());
    auto* node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(_header->list_head_offset));
    while (true) {
      if (node->is_free && node->size >= size_bytes) {
        // use the current node
        node->is_free = false;
        if (node->size - size_bytes > align_up(sizeof(AllocatorListNode))) {
          _m_insert_node(node,
                         offset_from_addr(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode))) +
                             static_cast<difference_type>(size_bytes),
                         node->size - size_bytes - align_up(sizeof(AllocatorListNode)));
          node->size = size_bytes;
        }
        auto data_offset = offset_from_addr(node) + static_cast<difference_type>(align_up(sizeof(AllocatorListNode)));
        return {data_offset, node->size};
      } else {
        if (node->next_offset == 0) {
          throw std::bad_alloc();
        }
        node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
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
    assert(_header->mutex.is_locked());
    difference_type next_offset = node->next_offset;
    node->next_offset = new_nodes_offset;
    auto p = offset_from_addr(node);
    new (addr_from_offset(new_nodes_offset)) AllocatorListNode{
        .size = new_nodes_size, .next_offset = next_offset, .prev_offset = offset_from_addr(node), .is_free = true};
    if (next_offset != invalid_offset) {
      auto* next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(next_offset));
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
    assert(_header->mutex.is_locked());
    if (node->next_offset == invalid_offset) {
      return;
    }
    size_type merged_size = node->size;
    auto* next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
    while (true) {
      if (next_node == nullptr) {
        node->next_offset = -1;
        break;
      }
      if (!next_node->is_free) {
        next_node->prev_offset = offset_from_addr(node);
        break;
      }
      merged_size += next_node->size + align_up(sizeof(AllocatorListNode));
      next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(next_node->next_offset));
    }
    node->size = merged_size;
    node->next_offset = offset_from_addr(next_node);
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
    assert(_header->mutex.is_locked());
    if (node->prev_offset == invalid_offset) {
      return;
    }
    size_type merged_size = node->size;
    auto* prev_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->prev_offset));
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
      prev_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(prev_node->prev_offset));
    }
    prev_next_node->size = merged_size;
    auto* next_node = reinterpret_cast<AllocatorListNode*>(addr_from_offset(node->next_offset));
    if (next_node != nullptr) {
      next_node->prev_offset = offset_from_addr(prev_next_node);
    }
    prev_next_node->next_offset = node->next_offset;
  }

 private:
  /// located in provided memory right before _memory. aligned at 16 bytes
  Header* _header = nullptr;
  /// located in provided memory right after aligned _header
  void* _memory = nullptr;
};

}  // namespace ipcpp::shm