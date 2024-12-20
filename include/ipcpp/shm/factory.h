/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/shared_memory_file.h>
#include <ipcpp/utils/utils.h>

#include <string>

namespace ipcpp::shm {

struct shared_memory {
  template <MappingType T_Mapping = MappingType::SINGLE>
  static std::expected<MappedMemory<T_Mapping>, std::error_code> open_or_create(
      std::string&& shm_id, const std::size_t size_bytes) {
    return MappedMemory<T_Mapping>::open_or_create(
        ipcpp::utils::path_from_shm_id(shm_id), size_bytes);
  }

  template <MappingType T_Mapping = MappingType::SINGLE>
  static std::expected<MappedMemory<T_Mapping>, std::error_code> open(std::string&& shm_id, const AccessMode access_mode = AccessMode::READ) {
    return MappedMemory<T_Mapping>::open(ipcpp::utils::path_from_shm_id(shm_id), access_mode);
  }
};

}  // namespace ipcpp::shm