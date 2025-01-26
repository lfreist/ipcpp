//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <ipcpp/utils/atomic.h>
#include <ipcpp/utils/numeric.h>

#include <ipcpp/shm/error.h>

#include <chrono>
#include <concepts>
#include <expected>
#include <ostream>
#include <functional>

namespace carry {

typedef atomic::largest_lock_free_uint_t uint_t;
typedef numeric::half_size_int_t<uint_t> uint_half_t;

static_assert(std::atomic<uint_t>::is_always_lock_free, "uint_t must be lock free");
static_assert(std::atomic<uint_half_t>::is_always_lock_free, "uint_half_t must be lock free");
static_assert(!std::is_void_v<uint_t>, "The system is not supported by carry: The system has no atomic integer types");
static_assert(
    std::numeric_limits<uint_t>::digits >= 16,
    "The system is not supported by carry: the largest atomic integer is too small (minimum required: 16 bit)");

enum class AccessMode {
  READ,
  WRITE,
};

enum class InitializationState : std::uint8_t {
  uninitialized = 0,
  initialization_in_progress = 1,
  initialized = 2
};

std::ostream& operator<<(std::ostream& os, AccessMode am);

std::ostream& operator<<(std::ostream& os, InitializationState am);

}  // namespace carry