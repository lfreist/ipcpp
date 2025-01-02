/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/shm/mapped_memory.h>

int main() {
  auto mm = ipcpp::shm::MappedMemory<ipcpp::shm::MappingType::SINGLE>::open("name");
  if (!mm.has_value()) {
    std::cerr << "Error" << std::endl;
    return 1;
  }
  return 0;
}