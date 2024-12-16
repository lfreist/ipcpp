/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>

#include <ipcpp/utils/utils.h>

namespace ipcpp::shm {

template <typename T_p>
class ring_buffer {
 public:
  typedef T_p value_type;

 private:
  struct _s_header {
    /// offset to the first byte usable for allocations/data
    std::ptrdiff_t start;
    /// offset to the bytes right after the last accessible byte
    std::ptrdiff_t end;
    /// index of the next T_p in the buffer to write to
    std::size_t next_index;
  };

public:
  static std::size_t required_bytes_for(const std::size_t num_elements) {
    return utils::align_up(sizeof(_s_header)) + num_elements * sizeof(T_p);
  }

 public:
  ring_buffer(std::uintptr_t start, const std::size_t size) {
    std::ptrdiff_t header_size = utils::align_up(sizeof(_s_header));
    _header = std::construct_at(reinterpret_cast<_s_header*>(start), header_size, size - header_size, 0);
    _buffer = std::span<T_p>(reinterpret_cast<value_type*>(start + header_size), (size - header_size) / sizeof(value_type));
  }

  explicit ring_buffer(std::uintptr_t start) : _header(reinterpret_cast<_s_header*>(start)) {
    const std::ptrdiff_t header_size = utils::align_up(sizeof(_s_header));
    _buffer = std::span<T_p>(reinterpret_cast<value_type*>(start + header_size), size());
  }

  ring_buffer(const ring_buffer& other) = default;
  ring_buffer(ring_buffer&& other) = default;

  ring_buffer& operator=(const ring_buffer& other) = default;
  ring_buffer& operator=(ring_buffer&& other) noexcept = default;

  template <typename... T_Args>
  value_type* emplace(T_Args&&... args) {
    if (_header->next_index >= size()) {
      _header->next_index = 0;
    }
    auto* addr = std::addressof(_buffer[_header->next_index]);
    // TODO: here happens a forcefully overwrite of memory without possibly needed destruction of the data.
    //       we need to check here that the data at addr are actually okay to overwrite!
    std::construct_at(addr, std::forward<T_Args>(args)...);
    ++_header->next_index;
    return addr;
  }

  template <typename... T_Args>
  value_type* emplace_at(const std::size_t idx, T_Args&&... args) {
    return std::construct_at(std::addressof(_buffer[idx % size()]), idx, std::forward<T_Args>(args)...);
  }

  std::size_t size() { return _m_num_bytes() / sizeof(value_type); }

  const value_type& operator[](const std::size_t idx) const { return _buffer[idx % size()]; }
  value_type& operator[](const std::size_t idx) { return _buffer[idx % size()]; }

  std::size_t get_index(value_type* p) {
    return p - _buffer.data();
  }

 private:
  std::size_t _m_num_bytes() { return _header->end - _header->start; }

 private:
  _s_header* _header = nullptr;
  std::span<value_type> _buffer;
};

}  // namespace ipcpp::shm