//
// Created by lfreist on 17/10/2024.
//

#include <thread>
#include <chrono>

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
  shm::Publisher<publisher::DomainSocketNotifier> publisher(std::move(expected_mapped_memory.value()), std::move(expected_notifier.value()));

  while (true) {
    std::string message("hi");
    //std::getline(std::cin, message);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto data = publisher.get_data(message.size() + sizeof(uint64_t));
    //publisher.write(&timestamp, sizeof(uint64_t));

    if (message == "exit") {
      data.set_exit();
      std::cout << "exiting..." << std::endl;
      break;
    }
    data.data<uint64_t>()[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::memcpy(data.data<uint8_t>().data() + sizeof(uint64_t), message.data(), message.size());
  }
}