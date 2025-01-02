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

std::unordered_map<std::string, std::shared_ptr<TopicEntry>> TopicRegistry::_topic_registry{};
std::mutex TopicRegistry::_mutex{};

// === TopicRegistry ===================================================================================================
// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<TopicEntry>, std::error_code> TopicRegistry::get_topic(const std::string& id, const std::size_t min_shm_size) {
  std::unique_lock lock(TopicRegistry::_mutex);
  if (min_shm_size == 0) {
    return open_topic(id);
  } else {
    return create_topic(id, min_shm_size);
  }
}

// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<TopicEntry>, std::error_code> TopicRegistry::open_topic(const std::string& id) {
  auto it = TopicRegistry::_topic_registry.find(id);
  if (it != TopicRegistry::_topic_registry.end()) {
    return it->second;
  }
  file_lock lock("global");
  lock.lock();
  auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::open(TopicEntry::shm_name(id));
  lock.unlock();
  if (!e_mm.has_value()) {
    return std::unexpected(e_mm.error());
  }
  auto topic = std::make_shared<TopicEntry>(TopicEntry(id, std::move(*e_mm)));
  TopicRegistry::_topic_registry[id] = topic;
  return topic;
}

// _____________________________________________________________________________________________________________________
std::expected<std::shared_ptr<TopicEntry>, std::error_code> TopicRegistry::create_topic(const std::string& id, const std::size_t min_shm_size) {
  auto it = TopicRegistry::_topic_registry.find(id);
  if (it != TopicRegistry::_topic_registry.end()) {
    return std::unexpected(std::error_code(1, std::system_category()));
  }
  file_lock lock("global");
  lock.lock();
  auto e_mm = shm::MappedMemory<shm::MappingType::SINGLE>::create(TopicEntry::shm_name(id), min_shm_size);
  lock.unlock();
  if (!e_mm.has_value()) {
    return std::unexpected(e_mm.error());
  }
  auto topic = std::make_shared<TopicEntry>(TopicEntry(id, std::move(*e_mm)));
  TopicRegistry::_topic_registry[id] = topic;
  return topic;
}

// === TopicEntry ===========================================================================================================
// _____________________________________________________________________________________________________________________

// _____________________________________________________________________________________________________________________
std::string TopicEntry::socket_name(std::string_view id) {
  return std::format("{}/ipcpp/{}.sock", std::filesystem::temp_directory_path().string(), id);
}

// _____________________________________________________________________________________________________________________
std::string TopicEntry::shm_name(std::string_view id) {
#ifdef IPCPP_UNIX
  return std::format("/{}.shm", id);
#elifdef IPCPP_WINDOWS
  return std::format("Global:{}.shm", id);
#else
  static_assert(false, "not implemented for current platform");
#endif
}

// _____________________________________________________________________________________________________________________
const std::string& TopicEntry::id() const { return _id; }

// _____________________________________________________________________________________________________________________
bool TopicEntry::operator==(const ipcpp::TopicEntry& other) const {
  return _id == other._id;
}

std::expected<std::shared_ptr<TopicEntry>, std::error_code> get_topic(const std::string& id, const std::size_t min_shm_size) {
  return TopicRegistry::get_topic(id, min_shm_size);
}

}  // namespace ipcpp