//
// Created by leon- on 19/04/2025.
//

#pragma once

#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>

#include <ipcpp/topic.h>

namespace ipcpp::ps {

template <CommunicationMode T_cm, typename T_p>
class RealTimeService {};

template <typename T_p>
class RealTimeService<CommunicationMode::IPC, T_p> {
 public:
  explicit RealTimeService(std::string_view name);

  std::expected<RealTimePublisher<T_p>, std::error_code> new_publisher() {
    if (auto* service = get_service(); service != nullptr) {
      // TODO: create Publisher
    }
    // no service shm initialized
    return std::unexpected<std::error_code>({1, std::generic_category()});
  }
  std::expected<RealTimeSubscriber<T_p>, std::error_code> new_subscriber() {
    if (auto* service = get_service(); service != nullptr) {
      // TODO: create Subscriber
    }
    // no service shm initialized
    return std::unexpected<std::error_code>({1, std::generic_category()});
  }

 public:
  // TODO <lfreist>: read stats...
  // TODO <lfreist>: managing functionality: clean up zombie workers etc

 private:
  // helper methods
 RealTimeInstanceData* get_service() {
   if (_service_shm && _service_shm->shm().size() >= sizeof(RealTimeInstanceData)) {
     return reinterpret_cast<RealTimeInstanceData*>(_service_shm->shm().addr());
   }
   return nullptr;
 }

 private:
  Topic _service_shm;
  Topic _data_shm;
};

template <typename T_p>
class RealTimeService<CommunicationMode::LOCAL, T_p> {};

// === Definitions =====================================================================================================


}