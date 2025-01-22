/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/shm/mapped_memory.h>

int main() {
  auto mm = carry::shm::MappedMemory<carry::shm::MappingType::SINGLE>::open("name");
  if (!mm.has_value()) {
    std::cerr << "Error" << std::endl;
    return 1;
  }
  return 0;
}