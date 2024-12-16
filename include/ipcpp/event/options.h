/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>

namespace ipcpp::event {

namespace notifier {

struct Options {
  std::size_t max_num_observers;
  std::size_t queue_size;
};

}

namespace observer {

}

}