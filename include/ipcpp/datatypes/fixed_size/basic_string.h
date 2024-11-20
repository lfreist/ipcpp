/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/datatypes/fixed_size/container_base.h>

#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipcpp::fixed_size {

template <std::size_t _N, class _Tp, class _Traits = std::char_traits<_Tp>>
class basic_string {
  static_assert(std::is_same_v<_Tp, typename _Traits::char_type>);

  typedef std::basic_string_view<_Tp, _Traits> sv_type;

 public:
  typedef _Traits traits_type;
  typedef _Tp value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static const size_type npos = static_cast<size_type>(-1);

 public:
  // --- constructors --------------------------------------------------------------------------------------------------
  basic_string() = default;
  explicit basic_string(value_type init_value) {
    traits_type::assign(data(), _N, init_value);
    _size = _N;
    _data[_size] = '\0';
  }

  template <typename InputIt>
  basic_string(InputIt first, InputIt last) {
    size_type size = (last - first > _N) ? _N : last - first;
    for (size_type i = 0; i < size; ++i) {
      _data[i] = *(first + i);
    }
    _size = size;
    _data(_size) = '\0';
  }

  basic_string(value_type* str, size_type count) {
    if (count > _N) {
      throw std::out_of_range("count " + std::to_string(count) + " is out of range. Max size is " + std::to_string(_N));
    }
    traits_type::copy(data(), str, count);
    _size = count;
    _data[_size] = '\0';
  }

  basic_string(std::nullptr_t) = delete;
  template <class StringViewLike>
  explicit basic_string(const StringViewLike& t) {
    size_type count = t.size();
    if (count > _N) {
      throw std::out_of_range("size " + std::to_string(count) + " is out of range. Max size is " + std::to_string(_N));
    }
    traits_type::copy(data(), t.data(), count);
    _size = count;
    _data[_size] = '\0';
  }

  basic_string(basic_string&& other) noexcept {
    traits_type::move(data(), other.data(), other._size + 1);
    std::swap(_size, other._size);
  }

  basic_string(const basic_string& other) {
    traits_type::copy(data(), other.data(), other._size + 1);
    _size = other._size;
    _data[_size] = '\0';
  }

  basic_string(const basic_string& other, size_type pos) {
    (data(), other.data() + pos, other._size + 1);
    _size = other._size;
    _data[_size] = '\0';
  }

  basic_string(const basic_string& other, size_type pos, size_type count) {
    traits_type::copy(data(), other.data() + pos, count + 1);
    _size = count;
    _data[_size] = '\0';
  }

  basic_string(std::initializer_list<value_type> ilist) {
    static_assert(ilist.size() > _N, "initializer list contains too many values");
    traits_type::copy(data(), data(ilist), ilist.size());
    _size = ilist.size();
    _data[_size] = '\0';
  }

  // --- operator=() ---------------------------------------------------------------------------------------------------
  basic_string& operator=(const basic_string& str) {
    if (this == &str) {
      return *this;
    }
    traits_type::copy(data(), str.data(), str.size() + 1);
    _size = str.size();
    return *this;
  }

  basic_string& operator=(basic_string&& str) noexcept {
    if (this == &str) {
      return *this;
    }
    traits_type::move(data(), str.data(), str.size() + 1);
    std::swap(_size, str._size);
    return *this;
  }

  basic_string& operator=(const value_type* t) {
    size_type size = traits_type::length(t);
    traits_type::copy(data(), t, size);
    _size = size;
    _data[_size] = '\0';
    return *this;
  }

  basic_string& operator=(value_type t) {
    _data[0] = t;
    _data[1] = 0;
    _size = 1;
    return *this;
  }

  template <class StringViewLike>
  basic_string& operator=(const StringViewLike& sv) {
    if (sv.size() > _N) {
      throw std::out_of_range("size " + std::to_string(sv.size()) + " is out of range. Max size is " + std::to_string(_N));
    }
    traits_type::copy(data(), sv.data(), sv.size());
    _size = sv.size();
    _data[_size] = '\0';
    return *this;
  }

  basic_string& operator=(std::nullptr_t) = delete;

  // --- assign() ------------------------------------------------------------------------------------------------------
  basic_string& assign(size_type count, value_type t) {
    if (count > _N) {
      throw std::out_of_range("count " + std::to_string(count) + "is out of range. Max size is " + std::to_string(_N));
    }
    traits_type::assign(data(), count, t);
    _size = count;
    _data[_size] = '\0';
    return *this;
  }

  basic_string& assign(const basic_string& str) {
    traits_type::copy(data(), str.data(), str.size() + 1);
    _size = str.size();
    return *this;
  }

  basic_string& assign(const basic_string& str, size_type pos, size_type count) {
    if (pos > str.size()) {
      throw std::out_of_range("pos " + std::to_string(pos) + " is out of range. Valid range is [0, " +
                              std::to_string(str.size() - 1) + "]");
    }
    if (count > str.size() - pos) {
      throw std::out_of_range("count " + std::to_string(count) + " is out of range. Valid range is [0, " +
                              std::to_string(str.size() - pos - 1) + "]");
    }
    traits_type::copy(data(), str.data() + pos, count + 1);
    _size = str.size();
    return *this;
  }

  basic_string& assign(value_type* t, size_type count) {
    if (count > _N) {
      throw std::out_of_range("count " + std::to_string(count) + " is out of range. Valid range is [0, " +
                              std::to_string(_N - 1) + "]");
    }
    traits_type::copy(data(), t, count);
    _size = count;
    _data[_size] = '\0';
    return *this;
  }

  basic_string& assign(value_type* t) {
    size_type size = traits_type::length(t);
    if (size > _N) {
      throw std::out_of_range("size " + std::to_string(size) + " is out of range. Valid range is [0, " +
                              std::to_string(_N - 1) + "]");
    }
    traits_type::copy(data(), t, size);
    _size = size;
    _data[_size] = '\0';
    return *this;
  }

  template <class InputIt>
  basic_string& assign(InputIt first, InputIt last) {
    size_type size = last - first;
    if (size > _N) {
      throw std::out_of_range("size " + std::to_string(size) + " is out of range. Valid range is [0, " +
                              std::to_string(_N - 1) + "]");
    }
    InputIt current = first;
    for (size_type i = 0; current != last; ++i, ++first) {
      _data[i] = *current;
    }
    _size = size;
    _data[_size] = '\0';
    return *this;
  }

  basic_string& assign(std::initializer_list<value_type> ilist) {
    static_assert(ilist.size() > _N, "initializer list contains too many values");
    traits_type::move(data(), data(ilist), ilist.size());
    _size = ilist.size();
    _data[_size] = '\0';
    return *this;
  }

  template <class StringViewLike>
  basic_string& assign(const StringViewLike& sv) {
    this->assign(sv.data(), sv.size());
    return *this;
  }

  template <class StringViewLike>
  basic_string& assign(const StringViewLike& sv, size_type pos, size_type count = npos) {
    if (count == npos) {
      count = sv.size() - pos;
    }
    if (count > sv.size() - pos) {
      throw std::out_of_range("count " + std::to_string(count) + " is out of range. Valid range is [0, " +
                              std::to_string(sv.size() - pos - 1) + "]");
    }
    this->assign(sv.data() + pos, count);
    return *this;
  }

  // TODO: implement: basic_string& assign_range();

  // --- element access ------------------------------------------------------------------------------------------------
  reference at(size_type index) {
    if (index > _size) {
      throw std::out_of_range("index " + std::to_string(index) + " is out of range. Valid range is [0, " +
                              std::to_string(_size - 1) + "]");
    }
    return _data[index];
  }

  const_reference at(size_type index) const {
    if (index > _size) {
      throw std::out_of_range("index " + std::to_string(index) + " is out of range. Valid range is [0, " +
                              std::to_string(_size - 1) + "]");
    }
    return _data[index];
  }

  reference operator[](size_type index) { return _data[index]; }

  const_reference operator[](size_type index) const { return _data[index]; }

  reference front() { return _data[0]; }
  const_reference front() const { return _data[0]; }

  reference back() { return _data[_size - 1]; }
  const_reference back() const { return _data[_size - 1]; }

  pointer data() { return _data; }
  const_pointer data() const { return _data; }

  const_pointer c_str() { return _data; }

  explicit operator std::basic_string_view<value_type, _Traits>() const noexcept {
    return std::basic_string_view(data(), size());
  }

  // --- iterator ------------------------------------------------------------------------------------------------------
  // TODO: implement

  // --- capacity ------------------------------------------------------------------------------------------------------
  [[nodiscard]] bool empty() const { return _size == 0; }

  [[nodiscard]] size_type size() const { return _size; }
  [[nodiscard]] size_type length() const { return _size; }

  [[nodiscard]] size_type max_size() const { return _N; }

  [[nodiscard]] size_type capacity() const { return _N; }

  // --- modifiers -----------------------------------------------------------------------------------------------------
  void clear() {
    _size = 0;
    _data[0] = '\0';
  }

  basic_string& insert(size_type index, size_type count, value_type t) {
    if (_size + count > _N) {
      throw std::out_of_range("out of range: count must be <= " + std::to_string(_N - _size));
    }
    traits_type::move(data() + index + count, data() + index, _size - index + 1);
    traits_type::assign(data() + index, count, t);
    _size += count;
    return *this;
  }

  basic_string& insert(size_type index, const value_type* s) {
    size_type size = traits_type::length(s);
    return insert(index, s, size);
  }

  basic_string& insert(size_type index, const value_type* s, size_type count) {
    if (_size + count > _N) {
      throw std::out_of_range("Out of range: size of s must be <= " + std::to_string(_N - _size));
    }
    traits_type::move(data() + index + count, data() + index, _size - index + 1);
    traits_type::copy(data() + index, s, count);
    _size += count;
    return *this;
  }

  template <size_type S>
  basic_string& insert(size_type index, const basic_string<S, value_type, traits_type>& str) {
    return insert(index, str.data(), str.size());
  }

  template <size_type S>
  basic_string& insert(size_type index, const basic_string<S, value_type, traits_type>& str, size_type s_index,
                       size_type count = npos) {
    if (count == npos) {
      return insert(index, str.data(), str.size());
    } else {
      return insert(index, str.data(), count);
    }
  }

  // todo: implement iterator inserters
  template <class StringViewLike>
  basic_string& insert(size_type index, const StringViewLike& sv) {
    return insert(index, sv.data(), sv.size());
  }

  template <class StringViewLike>
  basic_string& insert(size_type index, const StringViewLike& sv, size_type sv_index, size_type count = npos) {
    if (count == npos) {
      return insert(index, sv.data() + sv_index, sv.size() - sv_index);
    } else {
      return insert(index, sv.data() + sv_index, count);
    }
  }
  // todo: implement insert_range
  // todo: implement erase

  void push_back(value_type v) {
    _data[_size] = v;
    _size++;
    _data[_size] = '\0';
  }

  void pop_back() {
    _size--;
    _data[_size] = '\0';
  }

  basic_string& append(size_type count, value_type v) {
    if ((_size + count) > _N) {
      throw std::out_of_range("count " + std::to_string(count) +
                              " is out of range. count must be <= " + std::to_string(_N - _size));
    }
    traits_type::assign(data() + _size, count, v);
    _size += count;
    _data[_size] = '\0';
    return *this;
  }

  template <size_type S>
  basic_string& append(const basic_string<S, value_type, traits_type>& str) {
    return append(str.data(), str.size());
  }
  template <size_type S>
  basic_string& append(const basic_string<S, value_type, traits_type>& str, size_type pos, size_type count = npos) {
    if (count == npos) {
      return append(str.data() + pos, str.size() - pos);
    } else {
      return append(str.data() + pos, count);
    }
  }

  basic_string& append(const value_type* s, size_type count) {
    if ((_size + count) > _N) {
      throw std::out_of_range("count " + std::to_string(count) +
                              " is out of range. count must be <= " + std::to_string(_N - _size));
    }
    traits_type::copy(data() + _size, s, count);
    _size += count;
    _data[_size] = '\0';
    return *this;
  }

  basic_string& append(const value_type* s) {
    size_type size = traits_type::length(s);
    return append(s, size);
  }

  basic_string& append(std::initializer_list<value_type> ilist) { return append(data(ilist), ilist.size()); }

  template <class StringViewLike>
  basic_string& append(const StringViewLike& sv) {
    return append(sv.datat(), sv.size());
  }

  template <class StringViewLike>
  basic_string& append(const StringViewLike& sv, size_type pos, size_type count = npos) {
    if (count == npos) {
      return append(sv.data() + pos, sv.size() - pos);
    } else {
      return append(sv.data() + pos, count);
    }
  }
  // todo: implement append_range

  template <size_type S>
  basic_string& operator+=(const basic_string<S, value_type, traits_type>& str) {
    return append(str);
  }

  basic_string& operator+=(value_type v) {
    push_back(v);
    return *this;
  }

  basic_string& operator+=(const value_type* s) { return append(s); }

  basic_string& operator+=(std::initializer_list<value_type> ilist) { return append(std::move(ilist)); }

  template <class StringViewLike>
  basic_string& operator+=(const StringViewLike& sv) {
    return append(sv);
  }

  // todo: implement replace
  // todo: implement replace_with_range

  size_type copy(value_type* dest, size_type count, size_type pos = 0) {
    count = (count + pos) > _size ? _size - pos : count;
    traits_type::copy(dest, data() + pos, count);
    return count;
  }

  // --- search --------------------------------------------------------------------------------------------------------
  template <size_type S>
  size_type find(const basic_string<S, value_type, traits_type>& str, size_type pos = 0) const {
    return find(str.data(), pos, str.size());
  }

  size_type find(const value_type* s, size_type pos, size_type count) const {
    const size_type _size_ = this->size();

    if (count == 0) return pos <= _size_ ? pos : npos;
    if (pos >= _size_) return npos;

    const value_type elem0 = s[0];
    const value_type* const _data_ = data();
    const value_type* first = _data_ + pos;
    const value_type* const last = _data_ + _size_;
    size_type len = _size_ - pos;

    while (len >= count) {
      first = traits_type::find(first, len - count + 1, elem0);
      if (!first) return npos;
      if (traits_type::compare(first, s, count) == 0) return first - _data_;
      len = last - ++first;
    }
    return npos;
  }

  size_type find(const value_type* s, size_type count) const {
    return find(s, 0, count);
  }

  size_type find(value_type v, size_type pos = 0) const {
    return traits_type::find(data() + pos, _size - pos, v);
  }

  template <class StringViewLike>
  size_type find(const StringViewLike& sv, size_type pos = 0) const {
    return find(sv.data(), pos, sv.size());
  }

  // todo: implement rfind
  // todo: implement find_first_of
  // todo: implement find_first_not_of
  // todo: implement find_last_of
  // todo: implement find_last_not_of

  // --- operations ----------------------------------------------------------------------------------------------------
  int compare(const basic_string& str) {
    int r = traits_type::compare(data(), str.data(), _size);
    if (!r) {
      r = S_compare(size(), str.size());
    }
    return r;
  }

  int compare(size_type pos1, size_type count1, const basic_string& str) {
    sv_type sv(str);
    return sv_type(*this).substr(pos1, count1).compare(sv);
  }

  int compare(size_type pos1, size_type count1, const basic_string& str, size_type pos2, size_type count2) {
    sv_type sv(str);
    return sv_type(*this).substr(pos1, count1).compare(sv.substr(pos2, count2));
  }

  // todo: implement other compares

  basic_string substr(size_type pos = 0, size_type count = npos) { return {}; }



  protected:
  size_type _size = 0;
  value_type _data[_N + 1]{};

 private:
  static int
  S_compare(size_type n1, size_type n2) {
    const difference_type d = difference_type(n1 - n2);

    if (d > __gnu_cxx::__numeric_traits<int>::__max)
      return __gnu_cxx::__numeric_traits<int>::__max;
    else if (d < __gnu_cxx::__numeric_traits<int>::__min)
      return __gnu_cxx::__numeric_traits<int>::__min;
    else
      return int(d);
  }
};

}  // namespace ipcpp::fixed_size