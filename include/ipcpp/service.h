/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#pragma once

#include <cstring>

namespace carry {

enum class ServiceMode { local, ipc };

enum class PublishPolicy { fifo, real_time, lazy, eager };

enum class ServiceType { undefined, publish_subscribe, request_response, pipe, event };

constexpr std::uint16_t max_identifier_size = 128;

struct CommonServiceShmLayout {
  std::atomic<InitializationState> initialization_state = InitializationState::uninitialized;
  std::uint32_t version_tag = 0;
  ServiceType service_type = ServiceType::undefined;
  std::size_t data_type_size = 0;
  char identifier[max_identifier_size] = {0};

  template <typename T, ServiceType T_st>
  static std::error_code construct_at(std::uintptr_t addr, std::uint32_t version_tag, const std::string& identifier) {
    static_assert(T_st != ServiceType::undefined);
    if (identifier.size() > max_identifier_size) {
      // TODO: return actual error: identifier exceeds max size
      return std::error_code(1, std::system_category());
    }
    auto* layout = reinterpret_cast<CommonServiceShmLayout*>(addr);
    layout->version_tag = version_tag;
    layout->service_type = T_st;
    layout->data_type_size = sizeof(T);
    std::fill_n(layout->identifier, max_identifier_size, '\0');
    std::strncpy(layout->identifier, identifier.data(), identifier.size());
  }

  template <typename T, ServiceType T_st>
  static std::expected<CommonServiceShmLayout*, std::error_code> interpret_at(std::uintptr_t addr, std::uint32_t version_tag, const std::string& identifier) {
    static_assert(T_st != ServiceType::undefined);
    if (identifier.size() > max_identifier_size) {
      // TODO: return actual error: identifier exceeds max size
      return std::unexpected(std::error_code(1, std::system_category()));
    }
    auto* layout = reinterpret_cast<CommonServiceShmLayout*>(addr);
    if (layout->version_tag != version_tag) {
      // TODO: error: incompatible version
      return std::unexpected(std::error_code(2, std::system_category()));
    }
    if (layout->service_type != T_st) {
      // TODO: invalid service type
      return std::unexpected(std::error_code(3, std::system_category()));
    }
    if (layout->data_type_size != sizeof(T)) {
      // TODO: invalid type
      return std::unexpected(std::error_code(4, std::system_category()));
    }
    if (std::strncmp(layout->identifier, identifier.data(), identifier.size()) != 0) {
      // TODO: invalid identifier
      return std::unexpected(std::error_code(5, std::system_category()));
    }
    return layout;
  }
};

template <typename T, ServiceMode T_sm, ServiceType T_st>
class Service {};

}  // namespace carry