//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/error.h>
#include <ipcpp/types.h>

#include <chrono>
#include <cstdint>
#include <expected>

namespace ipcpp::shm {

/**
 * MemoryBlock represents a block within a SharedAddressSpace synchronized using the provided mechanism S.
 *  A MemoryBlock may uses the start of the MemoryBlock to store synchronization structures (mutex, rwlock, ...)
 */
template <template <AccessMode> typename View, typename S>
  requires SynchronizedData<S, View>
class MemoryBlock {
 public:
 public:
  MemoryBlock(void* start, std::size_t size);  // call S::initialize
  ~MemoryBlock();

  template <AccessMode A>
  std::expected<View<A>, error::AccessError> acquire();

  template <AccessMode A>
  std::expected<View<A>, error::AccessError> acquire(Duration auto timeout);

  template <AccessMode A, typename F, typename SpanT = uint8_t>
    requires std::is_invocable_r_v<void, F, SpanT>
  std::expected<std::size_t, error::AccessError> process(F&& func) {
    return std::unexpected(error::AccessError::UNKNOWN_LOCK_ERROR);
  }

  template <AccessMode A, typename F, typename SpanT = uint8_t>
    requires std::is_invocable_r_v<void, F, SpanT>
  std::expected<std::size_t, error::AccessError> process(F&& func, Duration auto timeout) {

    return std::unexpected(error::AccessError::UNKNOWN_LOCK_ERROR);
  }

 private:
  void* _start;
  std::size_t _size;
};

}  // namespace ipcpp::shm