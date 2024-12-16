/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::publish_subscribe {

enum class error_t {
  success = 0,
  unknown_error,
};

class error_category final : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override { return "ipcpp::publish_subscribe::error_t"; }
  [[nodiscard]] std::string message(int ev) const override {
    switch (static_cast<error_t>(ev)) {
      case error_t::success:
        return "success";
      case error_t::unknown_error:
        return "unknown_error";
    }
    return "unlisted_error";
  }
};

}  // namespace ipcpp::publish_subscribe