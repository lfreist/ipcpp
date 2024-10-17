//
// Created by lfreist on 17/10/2024.
//

#include <ipcpp/publisher/notifier.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>

#include <cstring>

namespace ipcpp::shm {

template <template <typename N> typename NotifierT, typename N>
class Publisher {
 public:
  Publisher(MappedMemory<MappingType::DOUBLE>&& mapped_memory, NotifierT<Notification>&& notifier) : _mapped_memory(std::move(mapped_memory)), _notifier(std::move(notifier)) {
    _notifier.enable_registration();
  }

  bool write(void* data, std::size_t size) {
    if (size > _mapped_memory.size()) {
      return false;
    }
    if (_offset > _mapped_memory.size()) {
      // move _offset into first mapping
      _offset -= _mapped_memory.size();
    }
    std::memcpy(_mapped_memory.data<uint8_t>() + _offset, data, size);
    Notification notification {
        .notification_type = notification::NotificationType::REGULAR,
        .data_addr = static_cast<uint8_t*>(_mapped_memory.shared_addr()) + _offset,
        .size = size
    };

    _notifier.notify_subscribers(notification);

    _offset += size;
    return true;
  }

 private:
  std::size_t _offset = 0;
  MappedMemory<MappingType::DOUBLE>&& _mapped_memory;
  NotifierT<Notification> _notifier;
};

}