//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/types.h>

namespace ipcpp::shm {

struct Notification {
  notification::NotificationType notification_type = notification::NotificationType::UNINITIALIZED;
  std::size_t offset = 0;
  std::size_t size = 0;
};

}