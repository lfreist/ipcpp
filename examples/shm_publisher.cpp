//
// Created by lfreist on 17/10/2024.
//

#include <ipcpp/notification/notifier_old.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/shm/notification.h>
#include <ipcpp/shm/publisher.h>
#include <ipcpp/utils/utils.h>

#include <chrono>
#include <thread>

#include "message.h"

int main(int argc, char** argv) {
  using namespace ipcpp;
  auto expected_notifier = publisher::DomainSocketNotifier<shm::Notification>::create("/tmp/ipcpp.sock");
  if (!expected_notifier.has_value()) {
    std::cerr << expected_notifier.error() << std::endl;
    return 1;
  }
  auto expected_shared_address_space = shm::SharedAddressSpace::create<AccessMode::WRITE>("/ipcpp_shm", 4096);
  if (!expected_shared_address_space.has_value()) {
    std::cerr << expected_shared_address_space.error() << std::endl;
    return 1;
  }
  auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::DOUBLE>::create<AccessMode::WRITE>(std::move(expected_shared_address_space.value()));
  if (!expected_mapped_memory.has_value()) {
    std::cerr << expected_mapped_memory.error() << std::endl;
    return 1;
  }
  shm::Publisher<publisher::DomainSocketNotifier> publisher(std::move(expected_mapped_memory.value()), std::move(expected_notifier.value()));

  while (true) {
    std::string message("hi");
    std::getline(std::cin, message);

    auto data = publisher.get_data(sizeof(Message));

    if (message == "exit") {
      data.set_exit();
      std::cout << "exiting..." << std::endl;
      break;
    }
    Message& msg = data.data<Message>()[0];
    msg.timestamp = utils::timestamp();
    msg.msg_size = message.size();
  }

  expected_shared_address_space.value().unlink();
}