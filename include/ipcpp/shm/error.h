/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <iostream>

namespace ipcpp::shm {

enum class error_t {
  success = 0,
  creation_error,  // shm_open for create fails
  resize_error,    // ftruncate fails
  open_error,      // shm_open for open fails
  file_not_found,
  mapping_error,  // mmap fails
  mapped_at_wrong_address,
  unknown_error,
};

class error_category final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override { return "ipcpp::shm::error_t"; }
  [[nodiscard]] std::string message(int ev) const override {
    switch (static_cast<error_t>(ev)) {
      case error_t::success:
        return "success";
      case error_t::creation_error:
        return "creation_error";
      case error_t::resize_error:
        return "resize_error";
      case error_t::open_error:
        return "open_error";
      case error_t::file_not_found:
        return "file_not_found";
      case error_t::mapping_error:
        return "mapping_error";
      case error_t::mapped_at_wrong_address:
        return "mapped_at_wrong_address";
      case error_t::unknown_error:
        return "unknown_error";
    }
    return "unlisted_error";
  }
};

}  // namespace ipcpp::shm