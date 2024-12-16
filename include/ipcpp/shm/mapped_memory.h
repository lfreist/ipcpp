//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/error.h>

#include <cassert>
#include <cstdint>
#include <memory>

#include <spdlog/spdlog.h>

namespace ipcpp::shm {

enum class MappingType { SINGLE, DOUBLE };

template <MappingType MT>
class MappedMemory {
 public:
  MappedMemory(MappedMemory&& other) noexcept : _shared_address_space(std::move(other._shared_address_space)) {
    std::swap(_mapped_region, other._mapped_region);
    std::swap(_size, other._size);
    std::swap(_total_size, other._total_size);
  }

  MappedMemory& operator=(MappedMemory&& other) noexcept {
    _shared_address_space = std::move(other._shared_address_space);
    std::swap(_mapped_region, other._mapped_region);
    std::swap(_size, other._size);
    std::swap(_total_size, other._total_size);
    return *this;
  }

  ~MappedMemory() {
    if (_mapped_region != 0) {
      msync(true);
      ::munmap(reinterpret_cast<void*>(_mapped_region), _total_size);
    }
    spdlog::debug("MappedMemory ({}) destroyed", _shared_address_space.fd());
  }

  template <AccessMode A>
  static std::expected<MappedMemory, error::MappingError> open(shared_memory_file&& shared_address_space) {
    if constexpr (MT == MappingType::SINGLE) {
      spdlog::debug("MappedMemory<MAPPING::SINGLE>::create<{}>(shared_memory_file={})", static_cast<int>(A), shared_address_space.fd());
      MappedMemory self(std::move(shared_address_space));
      self._size = self._shared_address_space.size();
      self._total_size = self._shared_address_space.size();
      if (auto result = MappedMemory::map_memory<A>(self._size, nullptr, self._shared_address_space.fd(), 0);
          result.has_value()) {
        self._mapped_region = reinterpret_cast<std::uintptr_t>(result.value());
      } else {
        return std::unexpected(result.error());
      }

      return self;
    } else if constexpr (MT == MappingType::DOUBLE) {
      spdlog::debug("MappedMemory<MAPPING::DOUBLE>::create<{}>(shared_memory_file={})", static_cast<int>(A), shared_address_space.fd());
      MappedMemory self(std::move(shared_address_space));
      self._size = self._shared_address_space.size();
      self._total_size = self._shared_address_space.size() + self._shared_address_space.size();
      auto double_mapping = map_memory<A>(self._total_size, nullptr, 0, 0);
      if (!double_mapping.has_value()) {
        return std::unexpected(double_mapping.error());
      }
      self._mapped_region = reinterpret_cast<std::uintptr_t>(double_mapping.value());
      if (auto first_mapping = map_memory<A>(self._size, self._mapped_region, self._shared_address_space.fd(), 0);
          !first_mapping.has_value()) {
        return std::unexpected(first_mapping.error());
      }
      auto second_mapping =
          map_memory<A>(self.size(), self._mapped_region + self.size(), self._shared_address_space.fd(), 0);
      if (!second_mapping.has_value()) {
        return std::unexpected(second_mapping.error());
      }
      return self;
    } else {
      spdlog::error("MappedMemory::create() called with invalid MappingType {}", static_cast<int>(MT));
      return std::unexpected(error::MappingError::UNKNOWN_MAPPING);
    }
  }

  static std::expected<MappedMemory, error::MappingError> open_or_create(std::string&& shm_id, const std::size_t size) {
    if constexpr (MT == MappingType::SINGLE) {
      spdlog::debug("MappedMemory<MAPPING::SINGLE>::create(shm_id='{}', size={})", std::string(shm_id), size);
      auto shm_result = shared_memory_file::create(std::move(shm_id), size);
      if (!shm_result.has_value()) {
        return std::unexpected(error::MappingError::SHM_ERROR);
      }
      return MappedMemory::open<AccessMode::WRITE>(std::move(shm_result.value()));
    } else if constexpr (MT == MappingType::DOUBLE) {
      spdlog::debug("MappedMemory<MAPPING::DOUBLE>::create(shm_id='{}', size={})", std::string(shm_id), size);
      auto shm_result = shared_memory_file::create(std::move(shm_id), size);
      if (!shm_result.has_value()) {
        return std::unexpected(error::MappingError::SHM_ERROR);
      }
      return MappedMemory::open<AccessMode::WRITE>(std::move(shm_result.value()));
    } else {
      spdlog::error("MappedMemory::create() called with invalid MappingType {}", static_cast<int>(MT));
      return std::unexpected(error::MappingError::UNKNOWN_MAPPING);
    }
  }

  template <AccessMode A>
  static std::expected<MappedMemory, error::MappingError> open(std::string&& shm_id) {
    if constexpr (MT == MappingType::SINGLE) {
      spdlog::debug("MappedMemory<MAPPING::SINGLE>::open(shm_id='{}')", std::string(shm_id));
      auto shm_result = shared_memory_file::open<A>(std::move(shm_id));
      if (!shm_result.has_value()) {
        return std::unexpected(error::MappingError::SHM_ERROR);
      }
      return MappedMemory::open<A>(std::move(shm_result.value()));
    } else if constexpr (MT == MappingType::DOUBLE) {
      spdlog::debug("MappedMemory<MAPPING::DOUBLE>::open(shm_id='{}')", std::string(shm_id));
      auto shm_result = shared_memory_file::open<A>(std::move(shm_id));
      if (!shm_result.has_value()) {
        return std::unexpected(error::MappingError::SHM_ERROR);
      }
      return MappedMemory::open<A>(std::move(shm_result.value()));
    } else {
      spdlog::error("MappedMemory::open() called with invalid MappingType {}", static_cast<int>(MT));
      return std::unexpected(error::MappingError::UNKNOWN_MAPPING);
    }
  }

  void msync(const bool sync) const {
    ::msync(reinterpret_cast<void*>(_mapped_region), _size, sync ? MS_SYNC : MS_ASYNC);
  }

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
  explicit MappedMemory(shared_memory_file&& shared_address_space)
      : _shared_address_space(std::move(shared_address_space)) {}

  template <AccessMode A>
  static std::expected<void*, error::MappingError> map_memory(std::size_t expected_size, void* start_addr, int fd,
                                                              std::size_t offset);

  std::uintptr_t _mapped_region = 0;
  std::size_t _size = 0;
  std::size_t _total_size = 0;
  shared_memory_file _shared_address_space;
};

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
template <MappingType MT>
template <AccessMode A>
std::expected<void*, error::MappingError> MappedMemory<MT>::map_memory(const std::size_t expected_size,
                                                                                   void* start_addr, const int fd,
                                                                                   const std::size_t offset) {
  int protect_flags = PROT_READ;
  if constexpr (A == AccessMode::WRITE) {
    protect_flags |= PROT_WRITE;
  }

  int flags{};

  if (start_addr) {
    flags |= MAP_FIXED;
  }

  if (fd) {
    flags |= MAP_SHARED_VALIDATE;
  } else {
    flags |= MAP_SHARED | MAP_ANONYMOUS;
  }

  void* const mapped_region = ::mmap(start_addr, expected_size, protect_flags, flags, fd, static_cast<__off_t>(offset));

  if (mapped_region == MAP_FAILED) {
    return std::unexpected(static_cast<error::MappingError>(errno));
  }

  if (start_addr && start_addr != mapped_region) {
    return std::unexpected(error::MappingError::WRONG_ADDRESS);
  }

  return mapped_region;
}

}  // namespace ipcpp::shm