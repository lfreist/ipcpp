#include <chrono>
#include <iostream>

#include <ipcpp/shm/data_view.h>
#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/memory_block.h>

int main() {
  auto shm = ipcpp::shm::SharedAddressSpace::create<ipcpp::AccessMode::WRITE>("/my_memory", 2048);

}