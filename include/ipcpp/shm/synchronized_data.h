//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <ipcpp/types.h>

namespace ipcpp::shm {

template <template <AccessMode A> typename View>
class RWLockSynchronizedData {
  static_assert(SynchronizedData<RWLockSynchronizedData<View>, View>);

 public:

 private:

};

}