/**
* Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

namespace carry {

enum class ServiceMode {
  local,
  ipc
};

enum class PublishPolicy {
  fifo,
  real_time,
  lazy,
  eager
};

enum class ServiceType {
  publish_subscribe,
  request_response,
  pipe,
  event
};

class Service_Interface {
 protected:
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _service_shm = std::nullopt;
  std::optional<shm::MappedMemory<shm::MappingType::SINGLE>> _data_shm = std::nullopt;
};

class ServiceRegistry {
 public:
  static std::expected<std::shared_ptr<Service_Interface>, std::error_code> get_service(const std::string& service_id) {
    if (_services.contains(service_id)) {
      return _services[service_id];
    }
    return std::unexpected(std::error_code(1, std::system_category()));
  }

  static std::error_code add_service(std::string service_id, std::shared_ptr<Service_Interface> service) {
    if (_services.contains(service_id)) {
      return std::error_code(1, std::system_category());
    }
    _services.emplace(std::move(service_id), std::move(service));
    return std::error_code(0, std::system_category());
  }

  static bool has_service(const std::string& service_id) {
    return _services.contains(service_id);
  }

 private:
  static std::unordered_map<std::string, std::shared_ptr<Service_Interface>> _services;
};

std::unordered_map<std::string, std::shared_ptr<Service_Interface>> ServiceRegistry::_services = {};

}