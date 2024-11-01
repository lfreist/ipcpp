//
// Created by lfreist on 17/10/2024.
//

#include <ipcpp/notification/notification.h>
#include <ipcpp/notification/notification_handler.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/subscriber.h>

#include "message.h"

int main(int argc, char** argv) {
  using namespace ipcpp;
  auto expected_notification_handler =
      subscriber::DomainSocketNotificationHandler<shm::Notification>::create("/tmp/ipcpp.sock");
  if (!expected_notification_handler.has_value()) {
    std::cerr << expected_notification_handler.error() << std::endl;
    return 1;
  }
  auto expected_shared_address_space = shm::SharedAddressSpace::create<AccessMode::READ>("/ipcpp_shm", 4096);
  if (!expected_shared_address_space.has_value()) {
    std::cerr << expected_shared_address_space.error() << std::endl;
    return 1;
  }
  auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::DOUBLE>::create<AccessMode::READ>(
      std::move(expected_shared_address_space.value()));
  if (!expected_mapped_memory.has_value()) {
    std::cerr << expected_mapped_memory.error() << std::endl;
    return 1;
  }
  shm::Subscriber<subscriber::DomainSocketNotificationHandler> subscriber(
      std::move(expected_mapped_memory.value()), std::move(expected_notification_handler.value()));
  while (true) {
    auto result = subscriber.receive(
        [](shm::Notification notification,
           shm::Subscriber<subscriber::DomainSocketNotificationHandler>::SharedMemory& shared_memory) {
          int64_t timestamp = utils::timestamp();
          Message message = shared_memory.data<Message>(notification.offset)[0];
          std::cout << "Message received:\n"
                    << " latency: " << timestamp - message.timestamp << "\n"
                    << " size   : " << message.msg_size << std::endl;
        });

    if (!result.has_value()) {
      std::cout << result.error() << std::endl;
      break;
    }
  }
  return 0;
}