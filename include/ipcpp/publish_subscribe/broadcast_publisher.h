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
#include <ipcpp/shm/chunk_allocator.h>
#include <ipcpp/shm/dynamic_allocator.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/utils.h>

namespace ipcpp::publish_subscribe {

struct Notification {
  int64_t timestamp;
  std::ptrdiff_t data_offset;
};

template <typename T,
          typename NotifierT =
              event::DomainSocketNotifier<Notification, ipcpp::event::SubscriptionResponse<SubscriptionResponseData>>>
class Broadcaster : public Publisher_I<T, NotifierT> {
  typedef Publisher_I<T, NotifierT> base_publisher;

 public:
  template <AccessMode A>
  class Data {
   public:
    explicit Data(shm::ChunkAllocator<DataContainer<T>>* allocator, DataContainer<T>* data_container)
        : _allocator(allocator), _data_container(data_container) {
      if constexpr (A == AccessMode::WRITE) {
        _data_container->shared_mutex.lock();
      } else if constexpr (A == AccessMode::READ) {
        _data_container->shared_mutex.shared_lock();
      }
    }

    ~Data() {
      if constexpr (A == AccessMode::WRITE) {
        _data_container->shared_mutex.unlock();
      } else if constexpr (A == AccessMode::READ) {
        _data_container->observer_count.fetch_sub(1);
        if (_data_container->observer_count.load() == 0) {
          _allocator->deallocate(_data_container);
        }
        _data_container->shared_mutex.shared_unlock();
      }
    }

   private:
    shm::ChunkAllocator<DataContainer<T>>* _allocator;
    DataContainer<T>* _data_container;
  };

 public:
  Broadcaster(Broadcaster&& other) noexcept
      : Publisher_I<T, NotifierT>(std::move(other)),
        _id(other._id),
        _mapped_memory(std::move(other._mapped_memory)),
        _mapped_memory_dyn(std::move(other._mapped_memory_dyn)),
        _list_allocator(std::move(other._list_allocator)) {}

  /*
  static std::expected<Broadcaster, int> create(std::string&& id, std::size_t max_num_clients, std::size_t shm_size) {
    auto notifier = NotifierT::create(std::string("/tmp/" + id + ".ipcpp.sock"), max_num_clients);
    if (!notifier.has_value()) {
      return std::unexpected(-1);
    }
    notifier.value().accept_subscriptions();
    auto mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.shm"), shm_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(-1);
    }
    Broadcaster self(std::move(notifier.value()), std::move(mapped_memory.value()));
    self._id = std::move(id);
    return self;
  }
   */

  static std::expected<Broadcaster, int> create(std::string&& id, std::size_t max_num_clients, std::size_t shm_size,
                                                std::size_t dyn_shm_size) {
    auto notifier = NotifierT::create(std::string("/tmp/" + id + ".ipcpp.sock"), max_num_clients);
    if (!notifier.has_value()) {
      return std::unexpected(-1);
    }
    notifier.value().accept_subscriptions();
    auto mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.shm"), shm_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(-1);
    }
    auto mapped_memory_dyn = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.dyn.shm"), dyn_shm_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(-1);
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
    int64_t timestamp = utils::timestamp();
    DataContainer<T>* shm_data = _list_allocator.allocate();
    new (shm_data) DataContainer<T>(data, base_publisher::_notifier.num_observers());
    Notification notification{.timestamp = timestamp, .data_offset = _list_allocator.ptr_to_index(shm_data)};
    notify_observers(notification);
  }

  void publish(T&& data) override {
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
    int64_t timestamp = utils::timestamp();
    DataContainer<T>* shm_data = _list_allocator.allocate();
    new (shm_data) DataContainer<T>(std::move(data), base_publisher::_notifier.num_observers());
    Notification notification{.timestamp = timestamp, .data_offset = _list_allocator.ptr_to_index(shm_data)};
    notify_observers(notification);
  }

  void publish(DataContainer<T>* data) {
    int64_t timestamp = utils::timestamp();
    Notification notification{.timestamp = timestamp, .data_offset = _list_allocator.ptr_to_index(data)};
    notify_observers(notification);
  }

  DataContainer<T>* get_raw_access() { return _list_allocator.allocate(); }

 private:
  explicit Broadcaster(NotifierT&& notifier, shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory,
                       shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory_dyn)
      : Publisher_I<T, NotifierT>(std::move(notifier)),
        _mapped_memory(std::move(mapped_memory)),
        _mapped_memory_dyn(std::move(mapped_memory_dyn)),
        _list_allocator(_mapped_memory.addr(), _mapped_memory.size()) {
    shm::DynamicAllocator<uint8_t>::initialize_factory(_mapped_memory_dyn.addr(), _mapped_memory_dyn.size());
    base_publisher::_notifier.set_response_data(
        SubscriptionResponseData{.list_size = _mapped_memory.size(), .heap_size = _mapped_memory_dyn.size()});
  }

  void notify_observers(typename NotifierT::notifier_base::notification_type notification) override {
    Publisher_I<T, NotifierT>::_notifier.notify_observers(notification);
  }

 private:
  std::string _id{};
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  shm::ChunkAllocator<DataContainer<T>> _list_allocator;
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory_dyn;
};

}  // namespace ipcpp::publish_subscribe