/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_CRITICAL 5
#define LOG_LEVEL_OFF 6

#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL LOG_LEVEL_INFO
#endif
#if LOGGING_LEVEL == LOG_LEVEL_OFF
#pragma message("Logging is disabled")
#else
#include <spdlog/spdlog.h>
#endif

namespace ipcpp::logging {

#if LOGGING_LEVEL == LOG_LEVEL_OFF

enum class level : int {
  trace = LOG_LEVEL_TRACE,
  debug = LOG_LEVEL_DEBUG,
  info = LOG_LEVEL_INFO,
  warn = LOG_LEVEL_WARN,
  err = LOG_LEVEL_ERROR,
  critical = LOG_LEVEL_CRITICAL,
  off = LOG_LEVEL_OFF,
  n_levels
};

#else

using level = spdlog::level::level_enum;

#endif

inline void set_level(level lvl) {
#if LOGGING_LEVEL != LOG_LEVEL_OFF
  spdlog::set_level(lvl);
#endif
}

template <typename T>
inline void trace(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_TRACE
  spdlog::trace(msg);
#endif
}

template <typename T>
inline void debug(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_DEBUG
  spdlog::debug(msg);
#endif
}

template <typename T>
inline void info(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_INFO
  spdlog::info(msg);
#endif
}

template <typename T>
inline void warn(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_WARN
  spdlog::warn(msg);
#endif
}

template <typename T>
inline void error(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_ERROR
  spdlog::error(msg);
#endif
}

template <typename T>
inline void critical(const T& msg) {
#if LOGGING_LEVEL <= LOG_LEVEL_CRITICAL
  spdlog::critical(msg);
#endif
}

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_TRACE
inline void trace(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::trace(s, std::forward<Args>(args)...);
}
#else
inline void trace(Args&&...) {}
#endif

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_DEBUG
inline void debug(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::debug(s, std::forward<Args>(args)...);
}
#else
inline void debug([[maybe_unused]] Args&&...) {}
#endif

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_INFO
inline void info(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::info(s, std::forward<Args>(args)...);
}
#else
inline void info([[maybe_unused]] Args&&...) {}
#endif

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_WARN
inline void warn(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::warn(s, std::forward<Args>(args)...);
}
#else
inline void warn([[maybe_unused]] Args&&...) {}
#endif

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_ERROR
inline void error(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::error(s, std::forward<Args>(args)...);
}
#else
inline void error([[maybe_unused]] Args&&...) {}
#endif

template <typename... Args>
#if LOGGING_LEVEL <= LOG_LEVEL_CRITICAL
inline void critical(fmt::format_string<Args...> s, Args&&... args) {
  spdlog::critical(s, std::forward<Args>(args)...);
}
#else
inline void critical([[maybe_unused]] Args&&...) {}
#endif

}  // namespace ipcpp::logging