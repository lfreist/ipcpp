/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/allocator.h>
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
    std::cerr << expected_mapped_memory.error() << std::endl;
    return 1;
  }

  auto& mapped_mem = expected_mapped_memory.value();

  SharedMemoryAllocator shared_allocator(mapped_mem.data<void>(), mapped_mem.size());

  using SharedIntVector = std::vector<int, SharedMemoryAllocatorAdapter<int>>;
  SharedMemoryAllocatorAdapter<int> vector_allocator(&shared_allocator);
  {
    SharedIntVector shared_vector(vector_allocator);
    SharedIntVector shared_vector2(vector_allocator);

    // Populate the vector
    shared_vector.push_back(10);
    shared_vector2.reserve(1000);
    shared_vector.push_back(20);
    shared_vector.push_back(30);

    // Access elements
    for (const auto& elem : shared_vector) {
      std::cout << elem << std::endl;
    }

  }
}