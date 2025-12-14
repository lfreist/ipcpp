//
// Created by leon- on 19/04/2025.
//

#pragma once

#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>

#include <ipcpp/topic.h>

namespace ipcpp::ps {

template <typename T_p>
class RealTimeService {
 public:
  explicit RealTimeService(std::string_view name);

  std::expected<RealTimePublisher<T_p>, std::error_code> new_publisher();
  std::expected<RealTimeSubscriber<T_p>, std::error_code> new_subscriber();

 public:
  // TODO <lfreist>: read stats...
  // TODO <lfreist>: managing functionality: clean up zombie workers etc

 private:
  Topic _service_shm;
  Topic _data_shm;
};

// === Definitions =====================================================================================================


}