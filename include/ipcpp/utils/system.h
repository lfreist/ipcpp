/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/utils/platform.h>

#include <concepts>
#include <cstdint>
#if defined(IPCPP_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(IPCPP_UNIX)
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace ipcpp::utils::system {

namespace internal {

size_t get_page_size(void) {
  static size_t page_size = 0;

  if (page_size == 0) {
#if defined(IPCPP_WINDOWS)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = (size_t)si.dwPageSize;
#elif defined(IPCPP_UNIX)
    page_size = sysconf(_SC_PAGESIZE);
#endif
  }

  return page_size;
}

}  // namespace internal

template <typename T>
  requires std::is_integral_v<T>
inline T round_up_to_pagesize(T val) {
  return (val + internal::get_page_size() - 1) & ~(internal::get_page_size() - 1);
}

inline bool is_process_alive(std::uint64_t pid) {
  if (pid == 0) return false;
#if defined(_WIN32)
  HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
  if (!h) return false;

  DWORD exit_code = 0;
  bool alive = GetExitCodeProcess(h, &exit_code) && exit_code == STILL_ACTIVE;

  CloseHandle(h);
  return alive;
#else
  if (kill(static_cast<pid_t>(pid), 0) == 0) return true;

  return errno != ESRCH;
#endif
}

}  // namespace ipcpp::utils::system