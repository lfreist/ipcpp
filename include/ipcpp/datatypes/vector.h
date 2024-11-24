/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/datatypes/ipcpp_iterator.h>
#include <ipcpp/shm/dynamic_allocator.h>
#include <ipcpp/datatypes/concepts.h>

#include <format>

namespace ipcpp {

template <typename T_p, typename T_Allocator = ipcpp::shm::DynamicAllocator<T_p>>
class vector {
 public:
  typedef T_p value_type;
  typedef T_Allocator allocator_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T_p& reference;
  typedef const T_p& const_reference;
  typedef T_p* pointer;
  typedef const T_p* const_pointer;
  typedef normal_iterator<typename allocator_type::difference_type, allocator_type> iterator;
  typedef _reverse_iterator<typename allocator_type::difference_type, allocator_type> reverse_iterator;

 public:
  // --- constructors --------------------------------------------------------------------------------------------------
  vector() noexcept : _m_data() {}
  explicit vector(size_type n) : _m_data() {
    _m_create_storage(n);
    _m_default_initialize(n);
  }
  vector(size_type n, const value_type& v) : _m_data() {
    _m_create_storage(n);
    _m_fill_initialize(n, v);
  }
  /*
  vector(size_type n, value_type v, const allocator_type& a) requires std::is_trivially_copyable_v<value_type> : _m_allocator(a) {
    _m_create_storage(n);
    _m_fill_initialize(n, v);
  }
  */
  vector(const vector& other) {
    _m_create_storage(other.size());
    _m_copy_initialize(other.size(), other.data());
  }
  template <typename T_Allocator2>
  requires std::is_same_v<typename allocator_type::value_type, typename T_Allocator2::value_type>
  explicit vector(const vector<value_type, T_Allocator2>& other) {
    _m_create_storage(other.size());
    _m_copy_initialize(other.size(), other.data());
  }
  vector(vector&&) noexcept = default;
  vector(std::initializer_list<value_type> l) : vector(l.begin(), l.end()) {}

  template <typename T_InputIt>
  requires forward_iterator<T_InputIt> || std::is_same_v<T_InputIt, const_pointer>
  vector(T_InputIt first, T_InputIt last) {
    _m_create_storage(last - first);
    _m_copy_initialize(first, last);
  }

  // --- destructor ----------------------------------------------------------------------------------------------------
  ~vector() noexcept {
    _m_destroy(begin(), end());
    _m_deallocate(get_allocator().addr_from_offset(_m_data._m_start), capacity());
  }

  // --- copy/move assignment ------------------------------------------------------------------------------------------

  // --- assign --------------------------------------------------------------------------------------------------------

  // --- allocator -----------------------------------------------------------------------------------------------------
  allocator_type get_allocator() const { return allocator_type::get_from_factory(); }

  // --- element access ------------------------------------------------------------------------------------------------
  reference operator[](size_type n) noexcept { return *(_m_start_ptr() + n); }
  const_reference operator[](size_type n) const noexcept { return *(_m_start_ptr() + n); }

  reference at(size_type n) {
    _m_range_check(n);
    return (*this)[n];
  }
  const_reference at(size_type n) const {
    _m_range_check(n);
    return (*this)[n];
  }

  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }

  reference back() { return *(end() - 1); }
  const_reference back() const { return *(end() - 1); }

  pointer data() noexcept { return _m_start_ptr(); }

  const_pointer data() const noexcept { return _m_start_ptr(); }

  // --- iterators -----------------------------------------------------------------------------------------------------
  iterator begin() noexcept { return iterator(_m_data._m_start, get_allocator()); }

  iterator end() noexcept { return iterator(_m_data._m_finish, get_allocator()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  iterator cbegin() const noexcept { return iterator(_m_data._m_start, get_allocator()); }

  iterator cend() const noexcept { return iterator(_m_data._m_finish, get_allocator()); }

  reverse_iterator crbegin() const noexcept { return reverse_iterator(cend()); }

  reverse_iterator crend() const noexcept { return reverse_iterator(cbegin()); }

  // --- capacity ------------------------------------------------------------------------------------------------------
  [[nodiscard]] size_type size() const noexcept { return size_type(_m_finish_ptr() - _m_start_ptr()); }

  [[nodiscard]] size_type max_size() const noexcept { return get_allocator().max_size(); }

  [[nodiscard]] size_type capacity() const noexcept { return size_type(_m_end_of_storage_ptr() - _m_start_ptr()); }

  [[nodiscard]] bool empty() const noexcept { return begin() == end(); }

  // --- modifiers -----------------------------------------------------------------------------------------------------
  void clear() {
    _m_destroy();
    _m_data._m_finish = _m_data._m_start;
  }

  void push_back(const value_type& v) {
    if (_m_data._m_finish != _m_data._m_end_of_storage) {
      new (get_allocator().addr_from_offset(_m_data._m_finish)) value_type(v);
      _m_data._m_finish += sizeof(value_type);
    } else {
      _m_realloc_append(v);
    }
  }
  void push_back(value_type&& v) { emplace_back(std::move(v)); }

  template <typename... T_Args>
  reference emplace_back(T_Args&&... args) {
    if (_m_data._m_finish != _m_data._m_end_of_storage) {
      new (get_allocator().addr_from_offset(_m_data._m_finish)) value_type(std::forward<T_Args>(args)...);
      _m_data._m_finish += sizeof(value_type);
    } else {
      _m_realloc_append(std::forward<T_Args>(args)...);
    }
    return back();
  }

  //

 private:
  difference_type _m_allocate(size_type n) {
    if (n == 0) {
      return -1;
    }
    difference_type data_index = get_allocator().allocate_get_index(n);
    return data_index;
  }
  void _m_deallocate(pointer p, size_type n) {
    if (p) {
      get_allocator().deallocate(p, n);
    }
  }

  pointer _m_start_ptr() const { return get_allocator().addr_from_offset(_m_data._m_start); }
  pointer _m_finish_ptr() const { return get_allocator().addr_from_offset(_m_data._m_finish); }
  pointer _m_end_of_storage_ptr() const { return get_allocator().addr_from_offset(_m_data._m_end_of_storage); }

  void _m_create_storage(size_type n) {
    _m_data._m_start = _m_allocate(n);
    _m_data._m_finish = _m_data._m_start;
    _m_data._m_end_of_storage = _m_data._m_start + (n * sizeof(value_type));
  }

  void _m_default_initialize(size_type n) {
    for (size_type i = 0; i < n; ++i) {
      new (_m_start_ptr() + i) value_type;
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  void _m_fill_initialize(size_type n, const value_type& v) {
    for (size_type i = 0; i < n; ++i) {
      new (_m_start_ptr() + i) value_type(v);
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  template <typename T_Iterator>
  void _m_copy_initialize(size_type n, T_Iterator first) {
    for (size_type i = 0; i < n; ++i) {
      new (_m_start_ptr() + i) value_type(first[i]);
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  template <typename T_Iterator>
  void _m_copy_initialize(T_Iterator first, T_Iterator last) {
    T_Iterator cur = first;
    size_type i = 0;
    for (; cur != last; ++i, ++cur) {
      new (_m_start_ptr() + i) value_type(*cur);
      _m_data._m_finish += sizeof(value_type);
    }
  }

  template <typename T_Iterator>
  value_type* _m_uninitialized_move(T_Iterator src_start, T_Iterator src_end, T_Iterator dst_start) {
    value_type* finish = std::addressof(*dst_start);
    for (T_Iterator cur = src_start; cur != src_end; ++cur, ++dst_start) {
      new (std::addressof(*dst_start)) value_type(std::move(*cur));
      finish++;
    }
    return finish;
  }

  /**
   * @brief Range check used by ipcpp::vector::at()
   */
  void _m_range_check(size_type n) const {
    if (n >= size()) {
      throw std::out_of_range(std::format("ipcpp::vector::_m_range_check({}): out of range: size is {}", n, size()));
    }
  }

  size_type _m_check_length(size_type n, const char* caller) const {
    if (max_size() - size() < n) {
      throw std::length_error(std::format("{}: length_error: max_size: {}, size: {}", caller, max_size(), size()));
    }
    const size_type len = size() + std::max(size(), n);
    return (len < size() || len > max_size()) ? max_size() : len;
  }

  template <typename... T_Args>
  void _m_realloc_append(T_Args&&... args) {
    const size_type len = _m_check_length(1u, "ipcpp::vector::_m_realloc_append");
    if (len <= 0) {
      std::unreachable();
    }
    pointer old_start = _m_start_ptr();
    pointer old_finish = _m_finish_ptr();
    const size_type elems = end() - begin();
    difference_type new_start(_m_allocate(len));
    value_type* new_finish_ptr = _m_uninitialized_move(old_start, old_finish, get_allocator().addr_from_offset(new_start));
    _m_construct_at(new_finish_ptr, std::forward<T_Args>(args)...);
    new_finish_ptr++;
    _m_deallocate(old_start, capacity());
    _m_data._m_start = new_start;
    _m_data._m_finish = get_allocator().offset_from_addr(new_finish_ptr);
    _m_data._m_end_of_storage = new_start + (len * sizeof(value_type));
  }

  template <typename... T_Args>
  void _m_construct_at(pointer addr, T_Args&&... args) {
    new (addr) value_type(std::forward<T_Args>(args)...);
  }

  template <typename T_Iterator>
  void _m_destroy(T_Iterator first, T_Iterator last) {
    for (; first != last; ++first) {
      (*first).~value_type();
    }
  }

 private:
  struct vector_data {
    difference_type _m_start;
    difference_type _m_finish;
    difference_type _m_end_of_storage;

    constexpr vector_data() noexcept : _m_start(-1), _m_finish(-1), _m_end_of_storage(-1) {}
    constexpr vector_data(vector_data&& other) noexcept
        : _m_start(other._m_start), _m_finish(other._m_finish), _m_end_of_storage(other._m_end_of_storage) {
      other._m_start = other._m_finish = other._m_end_of_storage = -1;
    }
    constexpr void _m_copy_data(const vector_data& other) noexcept {
      _m_start = other._m_start;
      _m_finish = other._m_finish;
      _m_end_of_storage = other._m_end_of_storage;
    }
    constexpr void _m_swap_data(vector_data& other) noexcept {
      vector_data tmp;
      tmp._m_copy_data(*this);
      _m_copy_data(other);
      other._m_copy_data(tmp);
    }
  };

  vector_data _m_data;
};

}  // namespace ipcpp