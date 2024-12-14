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

// === shared_memory_file ==============================================================================================
// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(std::string&& path, const std::size_t size) : _path(std::move(path)), _size(size) {}

// _____________________________________________________________________________________________________________________
shared_memory_file::~shared_memory_file() {
  if (_fd != -1) {
    close(_fd);
  }
}

// _____________________________________________________________________________________________________________________
shared_memory_file::shared_memory_file(shared_memory_file&& other) noexcept {
  _path = std::move(other._path);
  std::swap(_size, other._size);
  std::swap(_fd, other._fd);
}

// _____________________________________________________________________________________________________________________
int shared_memory_file::fd() const { return _fd; }

// _____________________________________________________________________________________________________________________
std::size_t shared_memory_file::size() const { return _size; }

// _____________________________________________________________________________________________________________________
void shared_memory_file::unlink() const { shm_unlink(_path.c_str()); }

}  // namespace ipcpp::shm