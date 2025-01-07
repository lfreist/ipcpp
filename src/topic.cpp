/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/topic.h>
#include <ipcpp/utils/platform.h>
#include <ipcpp/shm/mapped_memory.h>

#include <filesystem>
#include <format>

namespace ipcpp {

std::unordered_map<std::string, std::shared_ptr<ShmRegistryEntry>> ShmRegistry::_shm_registry{};
std::mutex ShmRegistry::_mutex{};

// === TopicRegistry ===================================================================================================
// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> ShmRegistry::get_shm_entry(const std::string& id, std::size_t min_shm_size) {
  std::unique_lock lock(ShmRegistry::_mutex);
  auto e_entry = open_shm(id);
  if (e_entry.has_value()) {
    return e_entry;
  } else {
    return create_shm(id, min_shm_size);
  }
}

// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> ShmRegistry::open_shm(const std::string& id) {
  auto it = ShmRegistry::_shm_registry.find(id);
  if (it != ShmRegistry::_shm_registry.end()) {
    return it->second;
  }
  file_lock lock("global");
  lock.lock();
  auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::open(ShmRegistryEntry::shm_name(id));
  lock.unlock();
  if (!e_mm.has_value()) {
    return std::unexpected(e_mm.error());
  }
  auto topic = std::make_shared<ShmRegistryEntry>(ShmRegistryEntry(id, std::move(*e_mm)));
  ShmRegistry::_shm_registry[id] = topic;
  return topic;
}

// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> ShmRegistry::create_shm(const std::string& id,
                                                                                          std::size_t min_shm_size) {
  auto it = ShmRegistry::_shm_registry.find(id);
  if (it != ShmRegistry::_shm_registry.end()) {
    return std::unexpected(std::error_code(1, std::system_category()));
  }
  file_lock lock("global");
  lock.lock();
  auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::create(ShmRegistryEntry::shm_name(id), min_shm_size);
  lock.unlock();
  if (!e_mm.has_value()) {
    return std::unexpected(e_mm.error());
  }
  auto topic = std::make_shared<ShmRegistryEntry>(ShmRegistryEntry(id, std::move(*e_mm)));
  ShmRegistry::_shm_registry[id] = topic;
  return topic;
}

// === TopicEntry ===========================================================================================================
// _____________________________________________________________________________________________________________________

// _____________________________________________________________________________________________________________________
std::string ShmRegistryEntry::shm_name(std::string_view id) {
#ifdef IPCPP_UNIX
  return std::format("/{}.shm", id);
#elifdef IPCPP_WINDOWS
  return std::format("Global:{}.shm", id);
#else
  static_assert(false, "not implemented for current platform");
#endif
}

// _____________________________________________________________________________________________________________________
const std::string& ShmRegistryEntry::id() const { return _id; }

// _____________________________________________________________________________________________________________________
bool ShmRegistryEntry::operator==(const ipcpp::ShmRegistryEntry& other) const {
  return _id == other._id;
}

std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> get_shm_entry(const std::string& id,
                                                                          std::size_t min_shm_size) {
  return ShmRegistry::get_shm_entry(id, min_shm_size);
}

}  // namespace ipcpp