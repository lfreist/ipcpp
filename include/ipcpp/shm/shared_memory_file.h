//
// Created by lfreist on 16/10/2024.
//

#pragma once
#include <fcntl.h>
#include <ipcpp/shm/error.h>
#include <ipcpp/types.h>
#include <sys/stat.h>

#include <iostream>

#ifdef _MSVC_LANG
#include <windows.h>
#endif

namespace ipcpp::shm {

/**
 * RAII class for opening/creating a shared memory file
*/
class shared_memory_file {
 public:
#ifdef __linux__
  typedef int native_handle_t;
#else
  typedef HANDLE native_handle_t;
#endif
 public:
  ~shared_memory_file();
  shared_memory_file(shared_memory_file&& other) noexcept;
  shared_memory_file& operator=(shared_memory_file&& other) noexcept;

  static std::expected<shared_memory_file, std::error_code> create(std::string&& path, std::size_t size);
  static std::expected<shared_memory_file, std::error_code> open(std::string&& path, AccessMode mode = AccessMode::WRITE);

  [[nodiscard]] std::string_view name() const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] int native_handle() const;

  void unlink() const;

 [[nodiscard]] AccessMode access_mode() const;

 private:
  shared_memory_file(std::string&& path, std::size_t size);

 private:
  AccessMode _access_mode = AccessMode::READ;
  std::string _path;
  /// fd for unix, HANDLE for windows
  native_handle_t _native_handle = reinterpret_cast<native_handle_t>(0);
  std::size_t _size = 0U;
};

}