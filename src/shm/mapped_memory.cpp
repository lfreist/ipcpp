/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/utils.h>
#include <ipcpp/utils/logging.h>

namespace ipcpp::shm {
// === member function definitions: platform unspecific implementation =================================================
// ___ Constructor _____________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::SINGLE>::MappedMemory(shared_memory_file&& shm_file) : _shm_file(std::move(shm_file)) {}

// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::DOUBLE>::MappedMemory(shared_memory_file&& shm_file) : _shm_file(std::move(shm_file)) {}

// ___ Move Constructor ________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::SINGLE>::MappedMemory(MappedMemory&& other) noexcept : _shm_file(std::move(other._shm_file)) {
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
}

// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::DOUBLE>::MappedMemory(MappedMemory&& other) noexcept : _shm_file(std::move(other._shm_file)) {
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
}

// ___ Move Assignment Operator ________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::SINGLE>& MappedMemory<MappingType::SINGLE>::operator=(
    MappedMemory<MappingType::SINGLE>&& other) noexcept {
  _shm_file = std::move(other._shm_file);
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
  return *this;
}

// _____________________________________________________________________________________________________________________
template <>
MappedMemory<MappingType::DOUBLE>& MappedMemory<MappingType::DOUBLE>::operator=(
    MappedMemory<MappingType::DOUBLE>&& other) noexcept {
  _shm_file = std::move(other._shm_file);
  std::swap(_mapped_region, other._mapped_region);
  std::swap(_size, other._size);
  std::swap(_total_size, other._total_size);
  return *this;
}

// ___ size ____________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
std::size_t MappedMemory<MappingType::SINGLE>::size() const {
  return _size;
}

// _____________________________________________________________________________________________________________________
template <>
std::size_t MappedMemory<MappingType::DOUBLE>::size() const {
  return _size;
}

// ___ open ____________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::SINGLE>, std::error_code> MappedMemory<MappingType::SINGLE>::open(
    shared_memory_file&& shm_file, const AccessMode access_mode) {
  if (shm_file.access_mode() == AccessMode::READ && access_mode == AccessMode::WRITE) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::access_error), error_category()));
  }
  logging::debug("MappedMemory<MAPPING::SINGLE>::create(shared_memory_file={}, access_mode={})", shm_file.name(),
                 static_cast<int>(access_mode));
  MappedMemory self(std::move(shm_file));
  self._size = self._shm_file.size();
  self._total_size = self._shm_file.size();
  if (auto result = _map_memory(self._size, 0, self._shm_file.native_handle(), 0, access_mode); result.has_value()) {
    self._mapped_region = result.value();
  } else {
    return std::unexpected(result.error());
  }

  return self;
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::SINGLE>, std::error_code> MappedMemory<MappingType::SINGLE>::open(
    std::string&& shm_id, const AccessMode access_mode) {
  logging::debug("MappedMemory<MAPPING::SINGLE>::open(shm_id='{}', access_mode={})", std::string(shm_id),
                 static_cast<int>(access_mode));
  auto shm_result = shared_memory_file::open(std::move(shm_id), access_mode);
  if (!shm_result.has_value()) {
    return std::unexpected(shm_result.error());
  }
  return open(std::move(shm_result.value()));
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::DOUBLE>, std::error_code> MappedMemory<MappingType::DOUBLE>::open(
    shared_memory_file&& shm_file, const AccessMode access_mode) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::open(shared_memory_file={}, access_mode={})", shm_file.name(),
                 static_cast<int>(access_mode));
  if (shm_file.access_mode() == AccessMode::READ && access_mode == AccessMode::WRITE) {
    return std::unexpected(std::error_code(static_cast<int>(error_t::access_error), error_category()));
  }
  MappedMemory self(std::move(shm_file));
  self._size = self._shm_file.size();
  self._total_size = self._shm_file.size() + self._shm_file.size();
  auto double_mapping = _map_memory(self._total_size, 0, 0, 0, AccessMode::WRITE);
  if (!double_mapping.has_value()) {
    return std::unexpected(double_mapping.error());
  }
  self._mapped_region = double_mapping.value();
  if (auto first_mapping = _map_memory(self._size, self._mapped_region, self._shm_file.native_handle(), 0, access_mode);
      !first_mapping.has_value()) {
    return std::unexpected(first_mapping.error());
  }
  if (auto second_mapping =
          _map_memory(self.size(), self._mapped_region + self.size(), self._shm_file.native_handle(), 0, access_mode);
      !second_mapping.has_value()) {
    return std::unexpected(second_mapping.error());
  }
  return self;
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::DOUBLE>, std::error_code> MappedMemory<MappingType::DOUBLE>::open(
    std::string&& shm_id, const AccessMode access_mode) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::open(shm_id='{}', access_mode={})", std::string(shm_id),
                 static_cast<int>(access_mode));
  auto shm_result = shared_memory_file::open(std::move(shm_id), access_mode);
  if (!shm_result.has_value()) {
    return std::unexpected(shm_result.error());
  }
  return open(std::move(shm_result.value()), access_mode);
}

// ___ open_or_create __________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::SINGLE>, std::error_code> MappedMemory<MappingType::SINGLE>::open_or_create(
    std::string&& shm_id, const std::size_t size) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::create(shm_id='{}', size={})", std::string(shm_id), size);
  auto shm_result = shared_memory_file::create(std::move(shm_id), size);
  if (!shm_result.has_value()) {
    return std::unexpected(shm_result.error());
  }
  return open(std::move(shm_result.value()), AccessMode::WRITE);
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::SINGLE>, std::error_code> MappedMemory<MappingType::SINGLE>::open_or_create(
    shared_memory_file&& shm_file) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::create(shm_file='{}')", shm_file.name());
  return open(std::move(shm_file), AccessMode::WRITE);
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::DOUBLE>, std::error_code> MappedMemory<MappingType::DOUBLE>::open_or_create(
    std::string&& shm_id, const std::size_t size) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::create(shm_id='{}', size={})", std::string(shm_id), size);
  auto shm_result = shared_memory_file::create(utils::path_from_shm_id(shm_id), size);
  if (!shm_result.has_value()) {
    return std::unexpected(shm_result.error());
  }
  return open(std::move(shm_result.value()), AccessMode::WRITE);
}

// _____________________________________________________________________________________________________________________
template <>
std::expected<MappedMemory<MappingType::DOUBLE>, std::error_code> MappedMemory<MappingType::DOUBLE>::open_or_create(
    shared_memory_file&& shm_file) {
  logging::debug("MappedMemory<MAPPING::DOUBLE>::create(shm_file='{}')", shm_file.name());
  return open(std::move(shm_file), AccessMode::WRITE);
}

// ___ release _________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::SINGLE>::release() noexcept {
  _mapped_region = 0;
  _size = 0;
  _total_size = 0;
}

// _____________________________________________________________________________________________________________________
template <>
void MappedMemory<MappingType::DOUBLE>::release() noexcept {
  _mapped_region = 0;
  _size = 0;
  _total_size = 0;
}

// ___ addr ____________________________________________________________________________________________________________
// _____________________________________________________________________________________________________________________
template <>
std::uintptr_t MappedMemory<MappingType::SINGLE>::addr() const {
  return _mapped_region;
}

// _____________________________________________________________________________________________________________________
template <>
std::uintptr_t MappedMemory<MappingType::DOUBLE>::addr() const {
  return _mapped_region;
}

}  // namespace ipcpp::shm
