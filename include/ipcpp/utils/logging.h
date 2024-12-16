/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#define LOG_LEVEL_OFF 0
#define LOG_LEVEL_TRACE 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_WARN 4
#define LOG_LEVEL_ERROR 5
#define LOG_LEVEL_CRITICAL 6

#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL LOG_LEVEL_INFO
#endif
#if LOGGING_LEVEL > LOG_LEVEL_OFF
#include <spdlog/spdlog.h>
#else
#pragma message("Logging is disabled");
#endif
#include "../../../cmake-build-release-clang/_deps/spdlog-src/include/spdlog/fmt/bundled/base.h"

namespace ipcpp::logging {

template <typename T>
inline void trace(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_TRACE
  spdlog::trace(msg);
#endif
}

template <typename T>
inline void debug(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_DEBUG
  spdlog::debug(msg);
#endif
}

template <typename T>
inline void info(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_INFO
  spdlog::info(msg);
#endif
}

template <typename T>
inline void warn(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_WARN
  spdlog::warn(msg);
#endif
}

template <typename T>
inline void error(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_ERROR
  spdlog::error(msg);
#endif
}

template <typename T>
inline void critical(const T& msg) {
#if LOGGING_LEVEL >= LOG_LEVEL_CRITICAL
  spdlog::critical(msg);
#endif
}

template <typename... Args>
inline void trace(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_TRACE
  spdlog::trace(s, std::forward<Args>(args)...);
#endif
}

template <typename... Args>
inline void debug(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_DEBUG
  spdlog::debug(s, std::forward<Args>(args)...);
#endif
}

template <typename... Args>
inline void info(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_INFO
  spdlog::info(s, std::forward<Args>(args)...);
#endif
}

template <typename... Args>
inline void warn(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_WARN
  spdlog::warn(s, std::forward<Args>(args)...);
#endif
}

template <typename... Args>
inline void error(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_ERROR
  spdlog::error(s, std::forward<Args>(args)...);
#endif
}

template <typename... Args>
inline void critical(fmt::format_string<Args...> s, Args&&... args) {
#if LOGGING_LEVEL >= LOG_LEVEL_CRITICAL
  spdlog::critical(s, std::forward<Args>(args)...);
#endif
}

}  // namespace ipcpp::logging