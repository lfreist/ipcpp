//
// Created by lfreist on 16/10/2024.
//

#pragma once

#include <functional>

#include <ipcpp/shm/error.h>
#include <ipcpp/types.h>

namespace ipcpp::shm {

template <typename T = uint8_t>
class MemoryView {
 public:
  MemoryView(void* start, std::size_t size, std::function<void(void)>& release_callback) : _data(static_cast<T*>(start), size), _release_callback(release_callback) {}
  ~MemoryView() {
    _release_callback();
  }

  std::span<T> data() {
    return _data;
  }

 private:
  std::span<T> _data;
  std::function<void(void)>& _release_callback;
};

}