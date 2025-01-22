#include <string>
#include <stdexcept>
#include <system_error>
#include <cerrno>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace carry {

class lock_file {
 public:
  typedef int native_handle;
 public:
  explicit lock_file(std::string file_path) : _file_path(std::move(file_path)) {
    _file_handle = open(_file_path.c_str(), O_CREAT | O_RDWR, 0666);
    if (_file_handle == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to open lock file");
    }
  }

  ~lock_file() {
    unlock();
    close(_file_handle);
  }

  bool try_lock() {
    if (_lock_owned) {
      return true;
    }
    if (_m_lock()) {
      _lock_owned = true;
    }
    return _lock_owned;
  }

  void unlock() {
    if (_lock_owned) {
      _m_unlock();
      _lock_owned = false;
    }
  }

 private:
  void _m_unlock() const {
    struct flock fl = {};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // Unlock the entire file
    if (fcntl(_file_handle, F_SETLK, &fl) == -1) {
      throw std::system_error(errno, std::generic_category(), "Failed to release lock");
    }
  }

  bool _m_lock() const {
    struct flock fl = {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // Lock the entire file
    if (fcntl(_file_handle, F_SETLK, &fl) == -1) {
      return false;
    }
    return true;
  }

 private:
  bool _lock_owned = false;
  std::string _file_path;
  native_handle _file_handle;
};

}