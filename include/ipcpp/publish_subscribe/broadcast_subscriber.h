/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/event/domain_socket_observer.h>
#include <ipcpp/event/notification.h>
#include <ipcpp/publish_subscribe/data_container.h>
#include <ipcpp/publish_subscribe/subscriber.h>
#include <ipcpp/publish_subscribe/subscription_respons.h>
#include <ipcpp/shm/chunk_allocator.h>
#include <ipcpp/shm/dynamic_allocator.h>
#include <ipcpp/utils/utils.h>

namespace ipcpp::publish_subscribe {

struct Notification {
  int64_t timestamp;
  std::ptrdiff_t data_offset;
};

template <typename T,
          typename ObserverT =
              event::DomainSocketObserver<Notification, ipcpp::event::SubscriptionResponse<SubscriptionResponseData>>>
class BroadcastSubscriber : Subscriber_I<T, ObserverT> {
  typedef Subscriber_I<T, ObserverT> base_subscriber;

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

    T& data() { return _data_container->data; }

   private:
    shm::ChunkAllocator<DataContainer<T>>* _allocator;
    DataContainer<T>* _data_container;
  };

 public:
  BroadcastSubscriber(BroadcastSubscriber&& other) noexcept
      : Subscriber_I<T, ObserverT>(std::move(other)),
        _id(other._id),
        _mapped_memory(std::move(other._mapped_memory)),
        _mapped_memory_dyn(std::move(other._mapped_memory_dyn)),
        _list_allocator(std::move(other._list_allocator)) {}

  static std::expected<BroadcastSubscriber, int> create(std::string&& id) {
    auto notifier = ObserverT::create(std::string("/tmp/" + id + ".ipcpp.sock"));
    if (!notifier.has_value()) {
      return std::unexpected(-1);
    }
    auto expected_result = notifier.value().subscribe();
    if (!expected_result.has_value()) {
      std::cerr << "error: " << expected_result.error() << std::endl;
      exit(1);
    }
    SubscriptionResponseData result = expected_result.value();
    std::cout << result.heap_size << std::endl;
    auto mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.shm"), result.list_size);
    if (!mapped_memory.has_value()) {
      return std::unexpected(-2);
    }
    auto mapped_memory_dyn = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
        std::string("/" + id + ".ipcpp.dyn.shm"), result.heap_size);
    if (!mapped_memory_dyn.has_value()) {
      return std::unexpected(-3);
    }
    BroadcastSubscriber self(std::move(notifier.value()), std::move(mapped_memory.value()),
                             std::move(mapped_memory_dyn.value()));
    self._id = std::move(id);
    return self;
  }

  void on_receive(std::function<void(Data<AccessMode::READ>& data)> callback) {
    auto expected_notification = base_subscriber::_observer.receive([](Notification n) { return n; });
    if (!expected_notification.has_value()) {
      std::cout << "received error" << std::endl;
      return;
    }
    std::int64_t timestamp_received = ipcpp::utils::timestamp();
    auto& notification = expected_notification.value();
    std::cout << "received data at timestamp: " << notification.timestamp << std::endl;
    std::cout << "                   latency: " << timestamp_received - notification.timestamp << std::endl;
    std::cout << "                    offset: " << notification.data_offset << std::endl;
    Data<AccessMode::READ> data(&_list_allocator, _list_allocator.index_to_ptr(notification.data_offset));
    callback(data);
  }

 private:
  BroadcastSubscriber(ObserverT&& observer, shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory,
                      shm::MappedMemory<shm::MappingType::SINGLE>&& mapped_memory_dyn)
      : Subscriber_I<T, ObserverT>(std::move(observer)),
        _mapped_memory(std::move(mapped_memory)),
        _mapped_memory_dyn(std::move(mapped_memory_dyn)),
        _list_allocator(_mapped_memory.addr(), _mapped_memory.size()) {
    shm::DynamicAllocator<uint8_t>::initialize_factory(_mapped_memory_dyn.addr());
  }

 private:
  std::string _id{};
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory;
  shm::ChunkAllocator<DataContainer<T>> _list_allocator;
  shm::MappedMemory<shm::MappingType::SINGLE> _mapped_memory_dyn;
};

}  // namespace ipcpp::publish_subscribe