//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/shm/error.h>
#include <ipcpp/shm/shared_memory_file.h>
#include <ipcpp/utils/logging.h>
#include <ipcpp/utils/platform.h>

#include <cassert>
#include <cstdint>
#include <memory>

#ifdef IPCPP_UNIX
#include <sys/mman.h>
#endif

namespace ipcpp::shm {

enum class MappingType { SINGLE, DOUBLE };

template <MappingType MT = MappingType::SINGLE>
class MappedMemory {
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

  void release() noexcept {
    _mapped_region = 0;
    _size = 0;
  }

  template <typename T>
  T* data(const std::size_t offset = 0) {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(addr()) + offset);
  }

  [[nodiscard]] std::size_t size() const { return _size; }

  [[nodiscard]] std::uintptr_t addr() const { return _mapped_region; }

 private:
  explicit MappedMemory(shared_memory_file&& shm_file);

  static std::expected<std::uintptr_t, std::error_code> map_memory(std::size_t expected_size, std::uintptr_t start_addr,
                                                                   shared_memory_file::native_handle_t file_handle, std::size_t offset, AccessMode access_mode);

  std::uintptr_t _mapped_region = 0;
  std::size_t _size = 0;
  std::size_t _total_size = 0;
  shared_memory_file _shm_file;
};

#ifdef __linux__
#include "internal/unix/mapped_memory.impl.h"
#endif

}  // namespace ipcpp::shm