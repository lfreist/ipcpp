//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/error.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <span>

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

  ~MappedMemory() {
    if (_mapped_region != nullptr) {
      msync(true);
      ::munmap(_mapped_region, _total_size);
    }
  }

  template <AccessMode A>
  static std::expected<MappedMemory, error::MappingError> create(SharedAddressSpace&& shared_address_space);

  template <AccessMode A>
  static std::expected<MappedMemory, error::MappingError> create(std::string&& shm_id, std::size_t size);

  void msync(const bool sync) const { ::msync(_mapped_region, _size, sync ? MS_SYNC : MS_ASYNC); }

  void release() noexcept {
    _mapped_region = nullptr;
    _size = 0;
  }

  template <typename T>
  T* data(const std::size_t offset = 0) {
    return static_cast<T*>(static_cast<uint8_t*>(_mapped_region) + offset);
  }

  [[nodiscard]] std::size_t size() const { return _size; }

  [[nodiscard]] void* shared_addr() const { return _shared_address_space.addr(); }

  [[nodiscard]] void* addr() const { return _mapped_region; }

 private:
  explicit MappedMemory(SharedAddressSpace&& shared_address_space)
      : _shared_address_space(std::move(shared_address_space)) {}

  template <AccessMode A>
  static std::expected<void*, error::MappingError> map_memory(std::size_t expected_size, void* start_addr, int fd,
                                                              std::size_t offset);

  void* _mapped_region = nullptr;
  std::size_t _size = 0;
  std::size_t _total_size = 0;
  SharedAddressSpace _shared_address_space;
};

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
template <>
template <AccessMode A>
std::expected<MappedMemory<MappingType::SINGLE>, error::MappingError> MappedMemory<MappingType::SINGLE>::create(
    SharedAddressSpace&& shared_address_space) {
  MappedMemory self(std::move(shared_address_space));

  self._size = self._shared_address_space.size();
  self._total_size = self._shared_address_space.size();
  auto result =
      MappedMemory::map_memory<A>(self._size, nullptr, self._shared_address_space.fd(), 0);
  if (result.has_value()) {
    self._mapped_region = result.value();
  } else {
    return std::unexpected(result.error());
  }

  return self;
}

// _____________________________________________________________________________________________________________________
template <>
template <AccessMode A>
std::expected<MappedMemory<MappingType::DOUBLE>, error::MappingError> MappedMemory<MappingType::DOUBLE>::create(
    SharedAddressSpace&& shared_address_space) {
  MappedMemory self(std::move(shared_address_space));

  self._size = self._shared_address_space.size();
  self._total_size = self._shared_address_space.size() + self._shared_address_space.size();
  auto double_mapping = map_memory<A>(self._total_size, nullptr, 0, 0);
  if (!double_mapping.has_value()) {
    return std::unexpected(double_mapping.error());
  }
  self._mapped_region = double_mapping.value();
  if (auto first_mapping = map_memory<A>(self._size, self._mapped_region, self._shared_address_space.fd(), 0);
      !first_mapping.has_value()) {
    return std::unexpected(first_mapping.error());
  }
  auto second_mapping = map_memory<A>(self._size, static_cast<uint8_t*>(self._mapped_region) + self._size,
                                      self._shared_address_space.fd(), 0);
  if (!second_mapping.has_value()) {
    return std::unexpected(second_mapping.error());
  }

  return self;
}

// _____________________________________________________________________________________________________________________
template <>
template <AccessMode A>
std::expected<MappedMemory<MappingType::SINGLE>, error::MappingError> MappedMemory<MappingType::SINGLE>::create(
    std::string&& shm_id, const std::size_t size) {
  auto shm_result = SharedAddressSpace::create<A>(std::move(shm_id), size);
  if (!shm_result.has_value()) {
    return std::unexpected(error::MappingError::SHM_ERROR);
  }
  return MappedMemory::create<A>(std::move(shm_result.value()));
}

// _____________________________________________________________________________________________________________________
template <>
template <AccessMode A>
std::expected<MappedMemory<MappingType::DOUBLE>, error::MappingError> MappedMemory<MappingType::DOUBLE>::create(
    std::string&& shm_id, const std::size_t size) {
  auto shm_result = SharedAddressSpace::create<A>(std::move(shm_id), size);
  if (!shm_result.has_value()) {
    return std::unexpected(error::MappingError::SHM_ERROR);
  }
  return MappedMemory<MappingType::DOUBLE>::create<A>(std::move(shm_result.value()));
}

// _____________________________________________________________________________________________________________________
template <MappingType MT>
template <AccessMode A>
std::expected<void*, error::MappingError> MappedMemory<MT>::map_memory(const std::size_t expected_size, void* start_addr, const int fd,
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