/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/shared_memory_file.h>
#include <ipcpp/shm/mapped_memory.h>

#include <string>

namespace ipcpp::shm {

struct shared_memory {
  template <MappingType T_Mapping = MappingType::SINGLE>
  static std::expected<MappedMemory<T_Mapping>, std::error_code> open_or_create(
      std::string&& shm_id, const std::size_t size_bytes) {
    return MappedMemory<T_Mapping>::open_or_create(
        std::move(shm_id), size_bytes);
  }

  template <MappingType T_Mapping = MappingType::SINGLE>
  static std::expected<MappedMemory<T_Mapping>, std::error_code> open(std::string&& shm_id, const AccessMode access_mode = AccessMode::READ) {
    return MappedMemory<T_Mapping>::open(std::move(shm_id), access_mode);
  }
};

}  // namespace ipcpp::shm