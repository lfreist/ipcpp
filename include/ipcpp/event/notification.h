//
// Created by lfreist on 17/10/2024.
//

#pragma once

namespace carry::event {

enum class ObserverRequest {
  SUBSCRIBE,
  CANCEL_SUBSCRIPTION,
  PAUSE_SUBSCRIPTION,
  RESUME_SUBSCRIPTION,
};

}  // namespace carry::event