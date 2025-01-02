/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
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

namespace ipcpp {


/**
 * @brief A topic is identified by an id provided in its constructor. ipcpp processes that use the name topic name share
 * the same shared memory and sockets.
 */
class TopicEntry final {
  friend class TopicRegistry;
 public:

  /**
   * @brief return a socket name/path
   * @return
   */
  static std::string socket_name(std::string_view id);

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

  bool operator==(const TopicEntry& other) const;

  explicit operator bool() const {
    return !_id.empty();
  }

  shm::MappedMemory<shm::MappingType::SINGLE>& shm() { return _manually_managed_mm; }

 private:
  TopicEntry(std::string id, shm::MappedMemory<shm::MappingType::SINGLE>&& mm) : _id(std::move(id)), _manually_managed_mm(std::move(mm)) {}

 private:
  /// topic id
  std::string _id;
  shm::MappedMemory<shm::MappingType::SINGLE> _manually_managed_mm;
};

struct TopicHash {
  std::size_t operator()(const TopicEntry& topic) const {
    return std::hash<std::string>()(topic.id());
  }
};

class TopicRegistry {
 public:
  static std::expected<std::shared_ptr<TopicEntry>, std::error_code> get_topic(const std::string& id, std::size_t min_shm_size = 0);

 private:
  static std::expected<std::shared_ptr<TopicEntry>, std::error_code> open_topic(const std::string& id);
  static std::expected<std::shared_ptr<TopicEntry>, std::error_code> create_topic(const std::string& id, std::size_t min_shm_size);

  static std::unordered_map<std::string, std::shared_ptr<TopicEntry>> _topic_registry;
  static std::mutex _mutex;  // safely access _topic_registry (intra process)
};

std::expected<std::shared_ptr<TopicEntry>, std::error_code> get_topic(const std::string& id, std::size_t min_shm_size = 0);

typedef std::shared_ptr<TopicEntry> Topic;

}  // namespace ipcpp