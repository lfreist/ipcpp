//
// Created by lfreist on 16/10/2024.
//

#include <fcntl.h>
#include <ipcpp/shm/address_space.h>
#include <ipcpp/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstddef>
#include <expected>
#include <functional>
#include <span>
#include <stdexcept>

// convert given argument to string at compile time
#define TO_STRING(x) #x

namespace ipcpp::shm {

// === SharedAddressSpace ==============================================================================================
// _____________________________________________________________________________________________________________________
SharedAddressSpace::SharedAddressSpace(std::string&& path, std::size_t size) : _path(std::move(path)), _size(size) {}

// _____________________________________________________________________________________________________________________
SharedAddressSpace::~SharedAddressSpace() {
  if (_start != nullptr) {
    munmap(_start, _size);
  }
  if (_fd != -1) {
    close(_fd);
  }
}

// _____________________________________________________________________________________________________________________
SharedAddressSpace::SharedAddressSpace(SharedAddressSpace&& other) noexcept {
  _path = std::move(other._path);
  std::swap(_start, other._start);
  std::swap(_size, other._size);
  std::swap(_fd, other._fd);
}

// _____________________________________________________________________________________________________________________
int SharedAddressSpace::fd() const { return _fd; }

// _____________________________________________________________________________________________________________________
std::size_t SharedAddressSpace::size() const { return _size; }

// _____________________________________________________________________________________________________________________
void* SharedAddressSpace::addr() const { return _start; }

// _____________________________________________________________________________________________________________________
void SharedAddressSpace::unlink() { shm_unlink(_path.c_str()); }

}  // namespace ipcpp::shm