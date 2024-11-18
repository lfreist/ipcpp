/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/address_space.h>
#include <ipcpp/shm/chunk_allocator.h>
#include <ipcpp/shm/mapped_memory.h>
#include <ipcpp/utils/utils.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

struct Data {
  int value;
  int64_t timestamp;
};

int main(int argc, char** argv) {
  auto expected_mapped_memory = ipcpp::shm::MappedMemory<ipcpp::shm::MappingType::SINGLE>::create<ipcpp::AccessMode::WRITE>("/list_alloc.ipcpp.shm", 1 << 20);
  if (!expected_mapped_memory.has_value()) {
    std::cout << "Error creating shared memory: " << (int)expected_mapped_memory.error() << std::endl;
  }
  auto& mapped_memory = expected_mapped_memory.value();
  ipcpp::shm::ChunkAllocator<Data> list_allocator(mapped_memory.addr(), mapped_memory.size(), 20);
  for (int i = 0; i < 30; ++i) {
    spdlog::info("allocating chunk {}", i);
    Data* data = list_allocator.allocate();
    data->timestamp = ipcpp::utils::timestamp();
    data->value = i;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}