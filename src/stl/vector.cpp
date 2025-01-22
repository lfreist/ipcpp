/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <ipcpp/stl/vector.h>
#include <ipcpp/stl/allocator.h>

namespace carry {

template class vector<int8_t>;
template class vector<int16_t>;
template class vector<int32_t>;
template class vector<int64_t>;
template class vector<uint8_t>;
template class vector<uint16_t>;
template class vector<uint32_t>;
template class vector<uint64_t>;

template class vector<double>;
template class vector<float>;

}