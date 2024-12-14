/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/domain_socket_notifier.h>
#include <ipcpp/publish_subscribe/publisher.h>
#include <ipcpp/publish_subscribe/subscription_respons.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/ring_buffer.h>
#include <ipcpp/stl/allocator.h>
#include <ipcpp/utils/reference_counted.h>
#include <ipcpp/utils/utils.h>
#include <ipcpp/publish_subscribe/notification.h>

#include <spdlog/spdlog.h>

namespace ipcpp::publish_subscribe {

template <typename T,
          typename NotifierT =
              event::DomainSocketNotifier<Notification, event::SubscriptionResponse<SubscriptionResponseData>>>
class Broadcaster final : public Publisher_I<T, NotifierT> {
  typedef Publisher_I<T, NotifierT> base_publisher;

 public:
  typedef T value_type;
  typedef reference_counted<T> value_access_type;

 public:
  Broadcaster(Broadcaster&& other) noexcept
      : Publisher_I<T, NotifierT>(std::move(other)),
        _id(std::move(other._id)),
        _mapped_memory(std::move(other._mapped_memory)),
        _message_buffer(std::move(other._message_buffer)),
        _mapped_memory_dyn(std::move(other._mapped_memory_dyn)) {}

  static std::expected<Broadcaster, factory_error> create(std::string&& id, std::size_t max_num_clients,
                                                          const std::size_t shm_size, const std::size_t dyn_shm_size) {
    auto notifier = NotifierT::create(std::string(id), max_num_clients);
    if (!notifier.has_value()) {
      return std::unexpected(factory_error::NOTIFIER_INITIALIZATION_FAILED);
    }
    notifier.value().accept_subscriptions();
    auto mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::open_or_create(
        std::string("/" + id + ".ipcpp.shm"), shm_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(factory_error::SHM_INITIALIZATION_FAILED);
    }
    auto mapped_memory_dyn = shm::MappedMemory<shm::MappingType::SINGLE>::open_or_create(
        std::string("/" + id + ".ipcpp.dyn.shm"), dyn_shm_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(factory_error::SHM_INITIALIZATION_FAILED);
    }
    Broadcaster self(std::move(notifier.value()), std::move(mapped_memory.value()),
                     std::move(mapped_memory_dyn.value()));
    self._id = std::move(id);
    return self;
  }

  /**
   * @brief Copies data to shared memory
   * @param data
   */
  void publish(T& data) override {
    static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
    if (base_publisher::_notifier.num_observers() == 0) {
      spdlog::warn("{}: no subscribers available", __func__);
    }
    const int64_t timestamp = utils::timestamp();
    const std::size_t index =
        _message_buffer.get_index(_message_buffer.emplace(data, base_publisher::_notifier.num_observers()));
    Notification notification{.timestamp = timestamp, .index = index};
    spdlog::debug("{}: published data at index {} by copy", __func__, index);
    notify_observers(notification);
  }

  void publish(T&& data) override {
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
    if (base_publisher::_notifier.num_observers() == 0) {
      spdlog::warn("{}: no subscribers available", __func__);
    }
    const int64_t timestamp = utils::timestamp();
    const std::size_t index =
        _message_buffer.get_index(_message_buffer.emplace(std::move(data), base_publisher::_notifier.num_observers()));
    Notification notification{.timestamp = timestamp, .index = index};
    spdlog::debug("{}: published data at index {} by move", __func__, index);
    notify_observers(notification);
  }

  void publish(value_access_type* data) {
    spdlog::debug("{}: publishing raw data", __func__);
    const int64_t timestamp = utils::timestamp();
    Notification notification{.timestamp = timestamp, .index = _message_buffer.get_index(data)};
    notify_observers(notification);
  }

  template <typename... T_Args>
  value_access_type* construct_and_get(T_Args&&... args) {
    spdlog::debug("{}: retrieving raw data", __func__);
    return _message_buffer.emplace(std::forward<T_Args>(args)...);
  }

 private:
  explicit Broadcaster(NotifierT&& notifier, shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory,
                       shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory_dyn)
      : Publisher_I<T, NotifierT>(std::move(notifier)),
        _mapped_memory(std::move(mapped_memory)),
        _message_buffer(reinterpret_cast<std::uintptr_t>(_mapped_memory.addr()), _mapped_memory.size()),
        _mapped_memory_dyn(std::move(mapped_memory_dyn)) {
    pool_allocator<uint8_t>::initialize_factory(reinterpret_cast<void*>(_mapped_memory_dyn.addr()), _mapped_memory_dyn.size());
    if constexpr (!std::is_void_v<typename base_publisher::notifier_type::notifier_base::subscription_return_type>) {
      base_publisher::_notifier.set_response_data(
          SubscriptionResponseData{.list_size = _mapped_memory.size(), .heap_size = _mapped_memory_dyn.size()});
    }
  }

  void notify_observers(typename NotifierT::notifier_base::notification_type notification) override {
    Publisher_I<T, NotifierT>::_notifier.notify_observers(notification);
  }

 private:
  std::string _id{};
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  shm::ring_buffer<reference_counted<T>> _message_buffer;
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory_dyn;
};

}  // namespace ipcpp::publish_subscribe