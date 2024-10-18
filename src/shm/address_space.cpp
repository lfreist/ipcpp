//
// Created by lfreist on 16/10/2024.
//

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <span>
#include <functional>
#include <cstddef>
#include <stdexcept>
#include <expected>
#include <chrono>

#include <ipcpp/shm/address_space.h>
#include <ipcpp/types.h>

// convert given argument to string at compile time
#define TO_STRING(x) #x

namespace ipcpp::shm {

// === SharedAddressSpace ==============================================================================================
SharedAddressSpace::SharedAddressSpace(std::string&& path, std::size_t size) : _path(std::move(path)), _size(size) {}

SharedAddressSpace::~SharedAddressSpace() {
  if (_start != nullptr) {
    munmap(_start, _size);
    shm_unlink(_path.c_str());
  }
  if (_fd != -1) {
    close(_fd);
  }
}

SharedAddressSpace::SharedAddressSpace(SharedAddressSpace&& other)  noexcept {
  _path = std::move(other._path);
  std::swap(_start, other._start);
  std::swap(_size, other._size);
  std::swap(_fd, other._fd);
}

}