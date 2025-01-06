/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/atomic.h>
#include <ipcpp/utils/numeric.h>

namespace ipcpp::ps {

typedef atomic::largest_lock_free_uint_t uint_t;
typedef numeric::half_size_int_t<uint_t> uint_half_t;

static_assert(std::atomic<uint_t>::is_always_lock_free, "uint_t must be lock free");
static_assert(std::atomic<uint_half_t>::is_always_lock_free, "uint_half_t must be lock free");
static_assert(!std::is_void_v<uint_t>, "The system is not supported by ipcpp: The system has no atomic integer types");
static_assert(
    std::numeric_limits<uint_t>::digits >= 16,
    "The system is not supported by ipcpp: the largest atomic integer is too small (minimum required: 16 bit)");

}  // namespace ipcpp::ps