/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 * 
 * This file is part of ipcpp.
 */

#pragma once

namespace ipcpp::publish_subscribe {

enum class factory_error {
  NOTIFIER_INITIALIZATION_FAILED,
  SHM_INITIALIZATION_FAILED,
};

}