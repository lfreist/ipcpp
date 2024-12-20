/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 *
 */

#pragma once

#include <ipcpp/utils/platform.h>
#include <ipcpp/shm/shared_memory_file.h>

#include <cstdint>
#include <expected>
#include <system_error>

namespace ipcpp::shm {

enum class MappingType { SINGLE, DOUBLE };

template <MappingType MT = MappingType::SINGLE>
class IPCPP_API MappedMemory {
 public:
  MappedMemory(MappedMemory&& other) noexcept;

  MappedMemory& operator=(MappedMemory&& other) noexcept;

  ~MappedMemory();

  static std::expected<MappedMemory, std::error_code> open_or_create(shared_memory_file&& shm_file);

  static std::expected<MappedMemory, std::error_code> open_or_create(std::string&& shm_id, std::size_t size);

  static std::expected<MappedMemory, std::error_code> open(shared_memory_file&& shm_file,
                                                           AccessMode access_mode = AccessMode::WRITE);

  static std::expected<MappedMemory, std::error_code> open(std::string&& shm_id,
                                                           AccessMode access_mode = AccessMode::READ);

  void msync(bool sync) const;

  void release() noexcept;

  [[nodiscard]] std::size_t size() const;

  [[nodiscard]] std::uintptr_t addr() const;

 private:
  explicit MappedMemory(shared_memory_file&& shm_file);

  std::uintptr_t _mapped_region = 0;
  std::size_t _size = 0;
  std::size_t _total_size = 0;
  shared_memory_file _shm_file;
};

// _____________________________________________________________________________________________________________________
std::expected<std::uintptr_t, std::error_code> IPCPP_API _map_memory(std::size_t expected_size, std::uintptr_t start_addr,
                                                           shared_memory_file::native_handle_t file_handle,
                                                           std::size_t offset, AccessMode access_mode);

}  // namespace ipcpp::shm