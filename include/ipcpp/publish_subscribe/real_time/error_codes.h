/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <system_error>

namespace ipcpp::ps::real_time::error {

enum class Subscriber {
  NoMessageAvailable,
  AcquireLimitExceeded
};

class SubscriberCategory final : public std::error_category {
 public:
  static SubscriberCategory& instance() {
    static SubscriberCategory sc;
    return sc;
  }

  [[nodiscard]] const char* name() const noexcept override { return "ipcpp::ps::real_time::error::Subscriber"; }
  [[nodiscard]] std::string message(int ev) const override {
    switch (static_cast<Subscriber>(ev)) {
      case Subscriber::NoMessageAvailable:
        return "no message available";
      case Subscriber::AcquireLimitExceeded:
        return "acquire limit exceeded";
    }
    return "unlisted_error";
  }
};

}