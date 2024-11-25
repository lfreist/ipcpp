/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/container/vector.h>
#include <ipcpp/shm/dynamic_allocator.h>

namespace ipcpp {

template class vector<int8_t , shm::DynamicAllocator<int8_t>>;
template class vector<int16_t, shm::DynamicAllocator<int16_t>>;
template class vector<int32_t, shm::DynamicAllocator<int32_t>>;
template class vector<int64_t, shm::DynamicAllocator<int64_t>>;
template class vector<uint8_t , shm::DynamicAllocator<uint8_t>>;
template class vector<uint16_t, shm::DynamicAllocator<uint16_t>>;
template class vector<uint32_t, shm::DynamicAllocator<uint32_t>>;
template class vector<uint64_t, shm::DynamicAllocator<uint64_t>>;

template class vector<double, shm::DynamicAllocator<double>>;
template class vector<float, shm::DynamicAllocator<float>>;

}