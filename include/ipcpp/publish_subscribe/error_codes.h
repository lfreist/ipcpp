/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::ps::error {

enum class MemoryInitialization {
  BufferTooSmall,   // allocated memory size is too small
  TooManyElements,  // max(uint_half_t) < maximum number of required indices
  InvalidInitializationState, // memory already initialized or in initialization or not initialized and tried to read
  CorruptedInitializationState, // failed to set state from in_initialization to initialized
  Timeout, // timeout waiting for buffer to be initialized
};

}