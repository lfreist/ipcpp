//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <memory>

#include <ipcpp/shm/address_space.h>

namespace ipcpp::shm {

enum class MappingType {
  SINGLE,
  DOUBLE
};

template <MappingType MT>
class MappedMemory {
 public:
  template <typename T>
  struct View {
    View(void* data, std::size_t size) : data((T*)(data), size) {}

    explicit operator bool() const { return data.size() != 0; }

    std::span<T> data;
  };

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

  static std::expected<MappedMemory, int> create(SharedAddressSpace&& shared_address_space);

  void msync(bool sync) {
    ::msync(_mapped_region, _size, sync ? MS_SYNC : MS_ASYNC);
  }

  void release() noexcept {
    _mapped_region = nullptr;
    _size = 0;
  }

  template <typename T>
  T* data() { return (T*)(_mapped_region); }

  [[nodiscard]] std::size_t size() const { return _size; }

  [[nodiscard]] void* shared_addr() const { return _shared_address_space.addr(); }

 private:
  MappedMemory(SharedAddressSpace&& shared_address_space) : _shared_address_space(std::move(shared_address_space)) {}

  static std::expected<void *, int> map_memory(std::size_t expected_size, void *start_addr, int fd, std::size_t offset) {
    [[maybe_unused]] const std::size_t page_size = getpagesize();

    const int protect_flags = PROT_READ | PROT_WRITE;

    int flags{};

    if (start_addr) {
      flags |= MAP_FIXED;
    }

    if (fd) {
      flags |= MAP_SHARED_VALIDATE;
    } else {
      flags |= MAP_SHARED | MAP_ANONYMOUS;
    }

    void *const mapped_region = ::mmap(start_addr, expected_size, protect_flags, flags, fd,
                                       static_cast<__off_t>(offset));

    if (mapped_region == MAP_FAILED) {
      throw std::runtime_error("Cannot mmap memory. Error: " + std::to_string(errno));
    }

    if (start_addr && start_addr != mapped_region) {
      throw std::runtime_error("Asked for a specific address, but mapped another");
    }

    return mapped_region;
  }

  void* _mapped_region = nullptr;
  std::size_t _size = 0;
  std::size_t _total_size = 0;
  SharedAddressSpace _shared_address_space;
};

template <>
std::expected<MappedMemory<ipcpp::shm::MappingType::SINGLE>, int> MappedMemory<MappingType::SINGLE>::create(ipcpp::shm::SharedAddressSpace&& shared_address_space) {
  MappedMemory<MappingType::SINGLE> self(std::move(shared_address_space));

  self._size = self._shared_address_space.size();
  self._total_size = self._shared_address_space.size();
  auto result = ipcpp::shm::MappedMemory<ipcpp::shm::MappingType::SINGLE>::map_memory(self._size, nullptr, self._shared_address_space.fd(), 0);
  if (result.has_value()) {
    self._mapped_region = result.value();
  } else {
    return std::unexpected(result.error());
  }

  return self;
}

template <>
std::expected<MappedMemory<ipcpp::shm::MappingType::DOUBLE>, int> MappedMemory<MappingType::DOUBLE>::create(ipcpp::shm::SharedAddressSpace&& shared_address_space) {
  MappedMemory<MappingType::DOUBLE> self(std::move(shared_address_space));

  self._size = self._shared_address_space.size();
  self._total_size = self._shared_address_space.size() + self._shared_address_space.size();
  auto double_mapping = map_memory(self._size, nullptr, 0, 0);
  if (!double_mapping.has_value()) {
    return std::unexpected(double_mapping.error());
  }
  self._mapped_region = double_mapping.value();
  auto first_mapping = map_memory(self._shared_address_space.size(), self._mapped_region, self._shared_address_space.fd(), 0);
  if (!first_mapping.has_value()) {
    return std::unexpected(first_mapping.error());
  }
  auto second_mapping = map_memory(self._shared_address_space.size(), static_cast<uint8_t*>(self._mapped_region) + self._shared_address_space.size(), self._shared_address_space.fd(), 0);
  if (!second_mapping.has_value()) {
    return std::unexpected(second_mapping.error());
  }

  return self;
}

}