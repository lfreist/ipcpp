//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/publisher/notifier.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>

#include <cstring>

namespace ipcpp::shm {

template <template <typename N> typename NotifierT>
class Publisher {
 public:
  class Data {
   public:
    Data(void* start, std::size_t offset, std::size_t size, NotifierT<Notification>* notifier) : _start(start), _offset(offset), _size(size), _notifier(notifier) {}
    ~Data() {
      if (_notifier) {
        Notification notification;
        notification.size = _size;
        notification.offset = _offset;
        notification.notification_type =
            _exit ? notification::NotificationType::EXIT : notification::NotificationType::REGULAR;
        _notifier->notify_subscribers(notification);
      }
    }

    Data(Data&& other) noexcept {
      std::swap(_start, other._start);
      std::swap(_offset, other._offset);
      std::swap(_size, other._size);
      std::swap(_notifier, other._notifier);
      std::swap(_exit, other._exit);
    }

    Data(const Data&) = delete;

    template <typename T>
    std::span<T> data() { return std::span((T*)(_start) + _offset, _size); }

    void set_exit() {
      _exit = true;
    }

    private:
    void* _start = nullptr;
    std::size_t _offset = 0;
    std::size_t _size = 0;
    NotifierT<Notification>* _notifier;
    bool _exit = false;
  };
 public:
  Publisher(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notifier) : _mapped_memory(std::move(mapped_memory)), _notifier(std::move(notifier)) {
    _notifier.enable_registration();
  }

  bool write(void* data, std::size_t size, bool exit = false) {
    if (size > _mapped_memory.size()) {
      return false;
    }
    if (_offset > _mapped_memory.size()) {
      // move _offset into first mapping
      _offset -= _mapped_memory.size();
    }
    std::memcpy(_mapped_memory.data<uint8_t>() + _offset, data, size);
    Notification notification {
        .notification_type = exit ? notification::NotificationType::EXIT : notification::NotificationType::REGULAR,
        .offset = _offset,
        .size = size,
    };

    _notifier.notify_subscribers(notification);

    _offset += size;
    return true;
  }

  Data get_data(std::size_t size) {
    if (size > _mapped_memory.size()) {
      return {nullptr, 0, 0, nullptr};
    }
    if (_offset > _mapped_memory.size()) {
      // move _offset into first mapping
      _offset -= _mapped_memory.size();
    }

    _offset += size;

    return {_mapped_memory.data<void>(), _offset - size, size, &_notifier};
  }

 private:
  std::size_t _offset = 0;
  MappedMemory<MappingType::DOUBLE>&& _mapped_memory;
  NotifierT<Notification> _notifier;
};

}