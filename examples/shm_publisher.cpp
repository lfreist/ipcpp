//
// Created by lfreist on 17/10/2024.
//

#include <ipcpp/shm/publisher.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>
#include <ipcpp/publisher/notifier.h>

int main(int argc, char** argv) {
  using namespace ipcpp;
  auto expected_notifier = publisher::DomainSocketNotifier<shm::Notification>::create("/tmp/ipcpp.sock");
  if (!expected_notifier.has_value()) {
    std::cerr << expected_notifier.error() << std::endl;
    return 1;
  }
  auto expected_shared_address_space = shm::SharedAddressSpace::create<AccessMode::WRITE>("/ipcpp_shm", 8096);
  if (!expected_shared_address_space.has_value()) {
    std::cerr << expected_shared_address_space.error() << std::endl;
    return 1;
  }
  auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::DOUBLE>::create(std::move(expected_shared_address_space.value()));
  if (!expected_mapped_memory.has_value()) {
    std::cerr << expected_mapped_memory.error() << std::endl;
    return 1;
  }
  shm::Publisher<publisher::DomainSocketNotifier, shm::Notification> publisher(std::move(expected_mapped_memory.value()), std::move(expected_notifier.value()));

  while (true) {
    std::string message;
    std::getline(std::cin, message);

    if (message == "exit") {
      std::cout << "exiting..." << std::endl;
      break;
    }

    publisher.write(message.data(), message.size());
  }
}