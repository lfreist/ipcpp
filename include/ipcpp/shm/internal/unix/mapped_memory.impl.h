/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 *
 * WARNING: Do NOT manually include anywhere!
 *
 */

// _____________________________________________________________________________________________________________________
template <MappingType MT>
MappedMemory<MT>::MappedMemory(shared_memory_file&& shm_file) : _shm_file(std::move(shm_file)) {}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
MappedMemory<MT>::MappedMemory(MappedMemory&& other) noexcept : _shm_file(std::move(other._shm_file)) {
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
MappedMemory<MT>::~MappedMemory() {
  if (_mapped_region != 0) {
    msync(true);
    ::munmap(reinterpret_cast<void*>(_mapped_region), _total_size);
  }
}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
MappedMemory<MT>& MappedMemory<MT>::operator=(MappedMemory<MT>&& other) noexcept {
  _shm_file = std::move(other._shm_file);
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
  return *this;
}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
void MappedMemory<MT>::msync(const bool sync) const {
  ::msync(reinterpret_cast<void*>(_mapped_region), _size, sync ? MS_SYNC : MS_ASYNC);
}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
std::expected<std::uintptr_t, std::error_code> MappedMemory<MT>::map_memory(const std::size_t expected_size,
                                                                            const std::uintptr_t start_addr,
                                                                            const int fd, const std::size_t offset,
                                                                            const AccessMode access_mode) {
  const int protect_flags = (access_mode == AccessMode::WRITE) ? PROT_READ | PROT_WRITE : PROT_READ;

  int flags = start_addr ? MAP_FIXED : 0;
  if (fd) {
    flags |= MAP_SHARED_VALIDATE;
  } else {
    flags |= MAP_SHARED | MAP_ANONYMOUS;
  }

  void* const mapped_region = ::mmap(reinterpret_cast<void*>(start_addr), expected_size, protect_flags, flags, fd,
                                     static_cast<__off_t>(offset));

  if (mapped_region == MAP_FAILED) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapping_error), error_category()));
  }

  if (reinterpret_cast<void*>(start_addr) && reinterpret_cast<void*>(start_addr) != mapped_region) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::mapped_at_wrong_address), error_category()));
  }

  return reinterpret_cast<std::uintptr_t>(mapped_region);
}