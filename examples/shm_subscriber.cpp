//
// Created by lfreist on 17/10/2024.
//

#include <ipcpp/shm/subscriber.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/subscriber/notification_handler.h>

int main(int argc, char** argv) {
  using namespace ipcpp;
  auto expected_notification_handler = subscriber::DomainSocketNotificationHandler<shm::Notification>::create("/tmp/ipcpp.sock");
  if (!expected_notification_handler.has_value()) {
    std::cerr << expected_notification_handler.error() << std::endl;
    return 1;
  }
  auto expected_shared_address_space = shm::SharedAddressSpace::create<AccessMode::WRITE>("/ipcpp_shm", 4096);
  if (!expected_shared_address_space.has_value()) {
    std::cerr << expected_shared_address_space.error() << std::endl;
    return 1;
  }
  auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::DOUBLE>::create(std::move(expected_shared_address_space.value()));
  if (!expected_mapped_memory.has_value()) {
    std::cerr << expected_mapped_memory.error() << std::endl;
    return 1;
  }
  shm::Subscriber subscriber(std::move(expected_mapped_memory.value()), std::move(expected_notification_handler.value()));
  subscriber.run();
  return 0;
}