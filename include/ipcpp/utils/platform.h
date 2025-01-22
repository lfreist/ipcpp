/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
* Author: Leon Freist <freist.leon@gmail.com>
*
* This file is part of carry.
*/

#pragma once

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__LINUX__)
#define IPCPP_UNIX
#endif
#if defined(__APPLE__)
#define IPCPP_APPLE
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define IPCPP_WINDOWS
#endif

#ifdef IPCPP_WINDOWS
#ifdef IPCPP_EXPORTS
#define IPCPP_API __declspec(dllexport)
#else
#ifdef IPCPP_IMPORTS
#define IPCPP_API __declspec(dllimport)
#else
#define IPCPP_API
#endif
#endif
#else
#define IPCPP_API
#endif