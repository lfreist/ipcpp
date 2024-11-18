//
// Created by lfreist on 17/10/2024.
//

#pragma once

#include <ipcpp/types.h>

namespace ipcpp::shm {

struct Notification {
  event::NotificationType notification_type = event::NotificationType::UNINITIALIZED;
  std::size_t offset = 0;
  std::size_t size = 0;
};

}