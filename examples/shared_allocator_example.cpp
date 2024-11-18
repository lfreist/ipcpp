/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/dynamic_allocator.h>
#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/mapped_memory.h>

int main(int argc, char** argv) {
  using namespace ipcpp;
  auto expected_shared_address_space = shm::SharedAddressSpace::create<AccessMode::WRITE>("/shared_alloc_shm", 1 << 27);
  if (!expected_shared_address_space.has_value()) {
    std::cerr << expected_shared_address_space.error() << std::endl;
    return 1;
  }
  auto expected_mapped_memory = shm::MappedMemory<shm::MappingType::SINGLE>::create<AccessMode::WRITE>(
      std::move(expected_shared_address_space.value()));
  if (!expected_mapped_memory.has_value()) {
    std::cerr << "error created mapped_memory" << std::endl;
    return 1;
  }

  auto& mapped_mem = expected_mapped_memory.value();

  ipcpp::shm::DynamicAllocator allocator(mapped_mem.addr(), mapped_mem.size());
  int* ints = allocator.allocate<int>(4);
  ints[0] = 0;
  ints[1] = 0;
  ints[2] = 0;
  ints[3] = 0;
  std::cout << reinterpret_cast<uint8_t*>(ints) - reinterpret_cast<uint8_t*>(mapped_mem.addr()) << std::endl;
  double* doubles = allocator.allocate<double>();
  *doubles = 0;
  std::cout << reinterpret_cast<uint8_t*>(doubles) - reinterpret_cast<uint8_t*>(ints) << std::endl;
  allocator.deallocate(ints);
  char* chars = allocator.allocate<char>(2);
  std::cout << reinterpret_cast<uint8_t*>(doubles) - reinterpret_cast<uint8_t*>(chars) << std::endl;
}