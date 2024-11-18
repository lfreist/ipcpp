//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/notification/notifier_old.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>

#include <cstring>

namespace ipcpp::shm {

template <template <typename N> typename NotifierT>
class Publisher {
 public:
  class Data {
   public:
    Data(void* start, std::size_t offset, std::size_t size, NotifierT<Notification>* notifier);
    ~Data();

    Data(Data&& other) noexcept;

    Data(const Data&) = delete;

    template <typename T>
    std::span<T> data();

    void* data();
    [[nodiscard]] std::size_t size() const;

    void set_exit();

   private:
    void* _start = nullptr;
    std::size_t _offset = 0;
    std::size_t _size = 0;
    NotifierT<Notification>* _notifier;
    bool _exit = false;
  };

 public:
  Publisher(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notifier);

  bool write(void* data, std::size_t size, bool exit = false);

  Data get_data(std::size_t size);

  void notify_subscribers(Notification&& notification);

 private:
  std::size_t _offset = 0;
  MappedMemory<MappingType::DOUBLE>&& _mapped_memory;
  NotifierT<Notification> _notifier;
};

// === implementations =================================================================================================
// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
Publisher<NotifierT>::Data::Data(void* start, std::size_t offset, std::size_t size, NotifierT<Notification>* notifier)
    : _start(start), _offset(offset), _size(size), _notifier(notifier) {}
// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
Publisher<NotifierT>::Data::~Data() {
  if (_notifier) {
    Notification notification;
    notification.size = _size;
    notification.offset = _offset;
    notification.notification_type =
        _exit ? event::NotificationType::EXIT : event::NotificationType::REGULAR;
    _notifier->notify_subscribers(notification);
  }
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
Publisher<NotifierT>::Data::Data(Publisher<NotifierT>::Data&& other) noexcept {
  std::swap(_start, other._start);
  std::swap(_offset, other._offset);
  std::swap(_size, other._size);
  std::swap(_notifier, other._notifier);
  std::swap(_exit, other._exit);
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
template <typename T>
std::span<T> Publisher<NotifierT>::Data::data() {
  return std::span((T*)((uint8_t*)(_start) + _offset), _size);
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
void Publisher<NotifierT>::Data::set_exit() {
  _exit = true;
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
void* Publisher<NotifierT>::Data::data() {
  return static_cast<uint8_t*>(_start) + _offset;
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
std::size_t Publisher<NotifierT>::Data::size() const {
  return _size;
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
Publisher<NotifierT>::Publisher(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notifier)
    : _mapped_memory(std::move(mapped_memory)), _notifier(std::move(notifier)) {
  _notifier.enable_registration();
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
bool Publisher<NotifierT>::write(void* data, std::size_t size, bool exit) {
  if (size > _mapped_memory.size()) {
    return false;
  }
  if (_offset > _mapped_memory.size()) {
    // move _offset into first mapping
    _offset -= _mapped_memory.size();
  }
  std::memcpy(_mapped_memory.data<uint8_t>() + _offset, data, size);
  Notification notification{
      .notification_type = exit ? event::NotificationType::EXIT : event::NotificationType::REGULAR,
      .offset = _offset,
      .size = size,
  };

  notify_subscribers(notification);

  _offset += size;
  return true;
}

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
Publisher<NotifierT>::Data Publisher<NotifierT>::get_data(std::size_t size) {
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

// _____________________________________________________________________________________________________________________
template <template <typename N> typename NotifierT>
void Publisher<NotifierT>::notify_subscribers(ipcpp::shm::Notification&& notification) {
  _notifier.notify_subscribers(notification);
}

}  // namespace ipcpp::shm