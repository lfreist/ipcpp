/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/utils.h>

#include <print>
#include <string>

namespace ipcpp::shm {

struct shared_memory {
  template <AccessMode T_Access, MappingType T_Mapping = MappingType::SINGLE>
  static MappedMemory<T_Mapping> open_or_create(std::string&& shm_id, const std::size_t size_bytes) {
    auto expected_mapped_memory = MappedMemory<T_Mapping>::template create<T_Access>(std::move(shm_id), size_bytes);
    if (!expected_mapped_memory) {
      throw std::runtime_error(std::format("Failed to open mapped memory with error {}", utils::to_string(expected_mapped_memory.error())));
    }
    return std::move(expected_mapped_memory.value());
  }
};

}  // namespace ipcpp::shm