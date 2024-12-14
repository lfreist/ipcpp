/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/notifier.h>
#include <ipcpp/event/shm_notification_memory_layout.h>
#include <ipcpp/shm/factory.h>
#include <ipcpp/utils/reference_counted.h>
#include <spdlog/spdlog.h>

#include <expected>

namespace ipcpp::event {

template <typename T_Notification>
class ShmAtomicNotifier final : public Notifier_I<T_Notification> {
 public:
  typedef shm_notification_memory_layout<reference_counted<T_Notification>, shm_atomic_notification_header>
      memory_layout;
  typedef Notifier_I<T_Notification> notifier_base;

 public:
  ShmAtomicNotifier(ShmAtomicNotifier&& other) noexcept
      : _mapped_memory(std::move(other._mapped_memory)), _memory_layout(std::move(other._memory_layout)) {
    spdlog::debug("ShmAtomicNotifier moved: {}", _memory_layout.header->is_active.load());
  }

  ShmAtomicNotifier(const ShmAtomicNotifier& other) = delete;

  ~ShmAtomicNotifier() override {
    if (_memory_layout.header == nullptr) {
      return;
    }
    _memory_layout.header->last_message_counter =
        _memory_layout.header->message_counter.load(std::memory_order_acquire);
    _memory_layout.header->message_counter.store(-1, std::memory_order_release);
    _memory_layout.header->is_active.store(false, std::memory_order_release);
    _mapped_memory.release();
  }

  static std::expected<ShmAtomicNotifier, factory_error> create(std::string&& id, const std::size_t shm_size_bytes) {
    auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::open_or_create(
        "/" + std::move(id) + ".nrb.ipcpp.shm", shm_size_bytes);
    if (!expected_mapped_memory.has_value()) {
      return std::unexpected(factory_error::SHM_CREATE_FAILED);
    }
    ShmAtomicNotifier self(std::move(expected_mapped_memory.value()));

    return self;
  }

  void notify_observers(typename notifier_base::notification_type notification) override {
    auto msg_number = _memory_layout.header->message_counter.load(std::memory_order_acquire) + 1;
    _memory_layout.message_buffer.emplace_at(msg_number, notification, num_observers());
    _memory_layout.header->message_counter.store(msg_number, std::memory_order_release);
    spdlog::debug("ShmAtomicNotifier::notify_observers (identifier: {}): published message #{}", _memory_layout.header->is_active.load(),
                  msg_number);
  }

  [[nodiscard]] std::size_t num_observers() const override { return _memory_layout.header->num_subscribers.load(); }

 private:
  explicit ShmAtomicNotifier(shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory)
      : _mapped_memory(std::move(mapped_memory)),
        _memory_layout(_mapped_memory.addr(), _mapped_memory.size()) {
    _memory_layout.header->is_active.store(true, std::memory_order_release);
    spdlog::debug("constructed ShmAtomicNotifier: {}", _memory_layout.header->is_active.load());
  }

 private:
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  memory_layout _memory_layout;
};

}  // namespace ipcpp::event