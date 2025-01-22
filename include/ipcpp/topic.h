/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <expected>
#include <system_error>
#include <mutex>

#include <ipcpp/utils/file_lock.h>
#include <ipcpp/shm/mapped_memory.h>

namespace carry {


/**
 * @brief A topic is identified by an id provided in its constructor. carry processes that use the name topic name share
 * the same shared memory and sockets.
 */
class ShmRegistryEntry final {
  friend class ShmRegistry;
 public:

  /**
   * @brief return the name/path of a shared memory object that is intended to be used as ring buffer
   * @return
   */
  static std::string shm_name(std::string_view id);

  /**
   * @brief returns original topic id
   * @return
   */
  [[nodiscard]] const std::string& id() const;

  bool operator==(const ShmRegistryEntry& other) const;

  explicit operator bool() const {
    return !_id.empty();
  }

  shm::MappedMemory<shm::MappingType::SINGLE>& operator*() { return _manually_managed_mm; }
  shm::MappedMemory<shm::MappingType::SINGLE>& shm() { return _manually_managed_mm; }
  shm::MappedMemory<shm::MappingType::SINGLE>* operator->() { return std::addressof(_manually_managed_mm); }

 private:
  ShmRegistryEntry(std::string id, shm::MappedMemory<shm::MappingType::SINGLE>&& mm) : _id(std::move(id)), _manually_managed_mm(std::move(mm)) {}

 private:
  /// topic id
  std::string _id;
  shm::MappedMemory<shm::MappingType::SINGLE> _manually_managed_mm;
};

struct TopicHash {
  std::size_t operator()(const ShmRegistryEntry& topic) const {
    return std::hash<std::string>()(topic.id());
  }
};

class ShmRegistry {
 public:
  static std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> get_shm_entry(const std::string& id, std::size_t min_shm_size = 0);

 private:
  static std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> open_shm(const std::string& id);
  static std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> create_shm(const std::string& id,
                                                                                      std::size_t min_shm_size);

  static std::unordered_map<std::string, std::shared_ptr<ShmRegistryEntry>> _shm_registry;
  static std::mutex _mutex;
};

std::expected<std::shared_ptr<ShmRegistryEntry>, std::error_code> get_shm_entry(const std::string& id, std::size_t min_shm_size = 0);

typedef std::shared_ptr<ShmRegistryEntry> ShmEntryPtr;

}  // namespace carry