/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/stl/alloc_traits.h>
#include <ipcpp/stl/allocator.h>
#include <ipcpp/stl/concepts.h>
#include <ipcpp/stl/ipcpp_iterator.h>

#include <concepts>
#include <format>
#include <utility>

namespace ipcpp {

template <typename T_p, typename T_Allocator = ipcpp::pool_allocator<T_p>>
class vector {
  // ___________________________________________________________________________________________________________________
 public:
  typedef allocator_traits<T_Allocator> alloc_traits;
  typedef T_p value_type;
  typedef T_Allocator allocator_type;
  typedef typename alloc_traits::size_type size_type;
  typedef typename alloc_traits::difference_type difference_type;
  typedef T_p& reference;
  typedef const T_p& const_reference;
  typedef T_p* pointer;
  typedef const T_p* const_pointer;
  typedef normal_iterator<pointer, vector> iterator;
  typedef normal_iterator<const_pointer, vector> const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;

  // ___________________________________________________________________________________________________________________
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
  vector(const vector& other)
    requires std::is_copy_constructible_v<value_type>
  {
    _m_create_storage(other.size());
    _m_copy_initialize(other.size(), other.data());
  }
  template <typename T_Allocator2>
    requires std::is_same_v<typename allocator_type::value_type, typename T_Allocator2::value_type>
  explicit vector(const vector<value_type, T_Allocator2>& other)
    requires std::is_copy_constructible_v<value_type>
  {
    _m_create_storage(other.size());
    _m_copy_initialize(other.size(), other.data());
  }
  vector(vector&&) noexcept = default;
  vector(std::initializer_list<value_type> l) : vector(l.begin(), l.end()) {}

  template <typename T_InputIt>
    requires forward_iterator<T_InputIt> || std::is_same_v<T_InputIt, const_pointer>
  vector(T_InputIt first, T_InputIt last) {
    size_type new_size;
    if constexpr (std::is_same_v<T_InputIt, decltype(begin())>) {
      new_size = last - first;
    } else {
      new_size = std::distance(first, last);
    }
    _m_create_storage(new_size);
    _m_copy_initialize(first, last);
  }

  // --- destructor ----------------------------------------------------------------------------------------------------
  ~vector() noexcept {
    _m_destroy(begin(), end());
    _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), capacity());
  }

  // --- copy/move assignment ------------------------------------------------------------------------------------------
  vector& operator=(const vector& other) {
    if (this == &other) {
      return *this;
    }
    // destroy old data
    _m_destroy(begin(), end());
    if (other.size() > capacity()) {
      // reallocate
      _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), size());
      _m_create_storage(other.size());
    }
    // create new
    _m_copy_initialize(other.size(), other.data());
    return *this;
  }
  vector& operator=(vector&& other) noexcept {
    _m_data._m_swap_data(other._m_data);
    return *this;
  }
  vector& operator=(std::initializer_list<value_type> ilist) {
    // destroy old data
    _m_destroy(begin(), end());
    if (ilist.size() > capacity()) {
      // reallocate
      _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), size());
      _m_create_storage(ilist.size());
    } else {
      _m_data._m_finish = _m_data._m_start;
    }
    // copy new data
    _m_copy_initialize(ilist.begin(), ilist.end());
    return *this;
  }

  // --- assign --------------------------------------------------------------------------------------------------------
  void assign(size_type count, const value_type& value) {
    // destroy old data
    _m_destroy(begin(), end());
    if (count > capacity()) {
      // reallocate
      _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), size());
      _m_create_storage(count);
    }
    _m_fill_initialize(count, value);
  }
  template <class T_InputIt>
    requires forward_iterator<T_InputIt> || std::is_same_v<T_InputIt, const_pointer>
  void assign(T_InputIt first, T_InputIt last) {
    // destroy old data
    _m_destroy(begin(), end());
    size_type new_size;
    if constexpr (std::is_same_v<T_InputIt, decltype(begin())>) {
      new_size = last - first;
    } else {
      new_size = std::distance(first, last);
    }
    if (new_size > capacity()) {
      // reallocate
      _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), size());
      _m_create_storage(new_size);
    }
    // copy new data
    _m_copy_initialize(first, last);
  }
  void assign(std::initializer_list<value_type> ilist) {
    // destroy old data
    _m_destroy(begin(), end());
    if (ilist.size() > capacity()) {
      // reallocate
      _m_deallocate(alloc_traits::offset_to_pointer(_m_data._m_start), size());
      _m_create_storage(ilist.size());
    }
    // copy new data
    _m_copy_initialize(ilist.begin(), ilist.end());
  }

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
  const_reference front() const { return *cbegin(); }

  reference back() { return *(end() - 1); }
  const_reference back() const { return *(cend() - 1); }

  pointer data() noexcept { return _m_start_ptr(); }

  const_pointer data() const noexcept { return _m_start_ptr(); }

  // --- iterators -----------------------------------------------------------------------------------------------------
  // NOTE: Any iterators cannot safely be shared across processes no matter what allocator is used!

  iterator begin() noexcept { return iterator(data()); }

  iterator end() noexcept { return iterator(data() + size()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_iterator cbegin() const noexcept { return const_iterator(data()); }

  const_iterator cend() const noexcept { return const_iterator(data() + size()); }

  const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

  const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

  // --- capacity ------------------------------------------------------------------------------------------------------
  [[nodiscard]] bool empty() const noexcept { return _m_data._m_start == _m_data._m_finish; }

  [[nodiscard]] size_type size() const noexcept { return size_type(_m_finish_ptr() - _m_start_ptr()); }

  [[nodiscard]] size_type max_size() const noexcept { return alloc_traits::max_size(); }

  void reserve(size_type new_cap) {
    if (new_cap <= capacity()) return;
    _m_realloc_copy(new_cap);
  }

  [[nodiscard]] size_type capacity() const noexcept { return size_type(_m_end_of_storage_ptr() - _m_start_ptr()); }

  void shrink_to_fit() {
    if (capacity() == size()) return;
    _m_realloc_copy(size());
  }

  // --- modifiers -----------------------------------------------------------------------------------------------------
  void clear() {
    _m_destroy(begin(), end());
    _m_data._m_finish = _m_data._m_start;
  }

  iterator insert(const_iterator pos, const value_type& value) { return insert(pos, 1, value); }
  iterator insert(const_iterator pos, value_type&& value) {
    value_type tmp(std::move(value));
    return insert(pos, tmp);
  }
  iterator insert(const_iterator pos, size_type count, const value_type& value) {
    difference_type offset = pos - cbegin();
    if (capacity() < size() + count) {
      // NOTE: _m_uninitialized_realloc_n may invalidate pos. Thus, we reset it to the same element after reallocation
      _m_uninitialized_realloc_n(count);
      pos = cbegin() + offset;
    }
    _m_move_to(pos, cend(), pos + count);
    for (size_type i = 0; i < count; ++i) {
      _m_construct_at(std::addressof(*(pos + i)), value);
      _m_data._m_finish += sizeof(value_type);
    }
    return begin() + offset;
  }
  template <class T_InputIt>
  iterator insert(const_iterator pos, T_InputIt first, T_InputIt last)
    requires std::input_iterator<T_InputIt>
  {
    difference_type offset = pos - cbegin();
    size_type count = std::distance(first, last);
    if (capacity() < size() + count) {
      // NOTE: _m_uninitialized_realloc_n may invalidate pos. Thus, we reset it to the same element after reallocation
      _m_uninitialized_realloc_n(count);
      pos = cbegin() + offset;
    }
    _m_move_to(pos, cend(), pos + count);
    for (size_type i = 0; i < count; ++i) {
      _m_construct_at(std::addressof(*(pos + i)), *first);
      first++;
      _m_data._m_finish += sizeof(value_type);
    }
    return begin() + offset;
  }
  iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) {
    return insert(pos, ilist.begin(), ilist.end());
  }

  // TODO: fix this:
  /*
  template <std::ranges::range T_Range>
  iterator insert_range(const_iterator pos, T_Range&& rg) {
    return insert(pos, rg.begin(), rg.end());
  }
   */

  template <class... T_Args>
  iterator emplace(const_iterator pos, T_Args&&... args) {
    return insert(pos, value_type(std::forward<T_Args>(args)...));
  }

  iterator erase(iterator pos) {
    if ((pos + 1) == end()) {
      std::destroy_at(std::addressof(*pos));
      _m_data._m_finish -= sizeof(value_type);
      return end();
    }
    difference_type offset = pos - begin();
    _m_move_to(pos + 1, end(), pos);
    _m_data._m_finish -= sizeof(value_type);
    return begin() + offset;
  }
  iterator erase(const_iterator pos) { return erase(iterator(pos)); }
  iterator erase(iterator first, iterator last) {
    difference_type offset = first - begin();
    difference_type len = last - first;
    if (len <= 0) {
      return last;
    }
    _m_move_to(last, end(), first);
    _m_data._m_finish -= len * sizeof(value_type);
    return begin() + offset;
  }
  iterator erase(const_iterator first, const_iterator last) { return erase(iterator(first), iterator(last)); }

  void push_back(const value_type& v) {
    if (_m_data._m_finish != _m_data._m_end_of_storage) {
      new (alloc_traits::offset_to_pointer(_m_data._m_finish)) value_type(v);
      _m_data._m_finish += sizeof(value_type);
    } else {
      _m_realloc_append(v);
    }
  }
  void push_back(value_type&& v) { emplace_back(std::move(v)); }

  template <typename... T_Args>
  reference emplace_back(T_Args&&... args) {
    if (_m_data._m_finish != _m_data._m_end_of_storage) {
      _m_construct_at(alloc_traits::offset_to_pointer(_m_data._m_finish), value_type(std::forward<T_Args>(args)...));
      _m_data._m_finish += sizeof(value_type);
    } else {
      _m_realloc_append(std::forward<T_Args>(args)...);
    }
    return back();
  }

  void pop_back() {
    std::destroy_at(&back());
    _m_data._m_finish -= sizeof(value_type);
  }

  void resize(size_type count) {
    if (count == size()) {
      return;
    } else if (count < size()) {
      _m_destroy(begin() + count, end());
      _m_data._m_finish = _m_data._m_start + (count * sizeof(value_type));
    } else {
      // count > size()
      size_type new_size = count;
      if (new_size <= capacity()) {
        // allocated memory can be filled
        for (size_type i = size(); i < new_size; ++i) {
          _m_construct_at(_m_start_ptr() + i);
        }
        _m_data._m_finish = _m_data._m_start + (new_size * sizeof(value_type));
      } else {
        // reallocate
        _m_realloc_append_n(new_size);
      }
    }
  }
  void resize(size_type count, const value_type& value) {
    if (count == size()) {
      return;
    } else if (count < size()) {
      _m_destroy(begin() + count, end());
      _m_data._m_finish = _m_data._m_start + (count * sizeof(value_type));
    } else {
      // count > size()
      size_type new_size = count;
      if (new_size <= capacity()) {
        // allocated memory can be filled
        for (size_type i = size(); i < new_size; ++i) {
          _m_construct_at(_m_start_ptr() + i, value);
        }
        _m_data._m_finish = _m_data._m_start + (new_size * sizeof(value_type));
      } else {
        // reallocate
        _m_realloc_append_n(new_size, value);
      }
    }
  }

  void swap(vector& other) noexcept { _m_data._m_swap_data(other._m_data); }

 private:  // ----------------------------------------------------------------------------------------------------------
  std::pair<difference_type, size_type> _m_allocate(size_type n) {
    if (n == 0) {
      return {-1, 0};
    }
    return alloc_traits::allocate_at_least(n);
  }
  void _m_deallocate(pointer p, size_type n) {
    if (p) {
      alloc_traits::deallocate(p, n);
    }
  }

  pointer _m_start_ptr() const { return alloc_traits::offset_to_pointer(_m_data._m_start); }
  pointer _m_finish_ptr() const { return alloc_traits::offset_to_pointer(_m_data._m_finish); }
  pointer _m_end_of_storage_ptr() const { return alloc_traits::offset_to_pointer(_m_data._m_end_of_storage); }

  void _m_create_storage(size_type n) {
    auto [start, size] = _m_allocate(n);
    _m_data._m_start = start;
    _m_data._m_finish = _m_data._m_start;
    _m_data._m_end_of_storage = _m_data._m_start + size;
  }

  void _m_default_initialize(size_type n) {
    for (size_type i = 0; i < n; ++i) {
      _m_construct_at(_m_start_ptr() + i);
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  void _m_fill_initialize(size_type n, const value_type& v) {
    for (size_type i = 0; i < n; ++i) {
      _m_construct_at(_m_start_ptr() + i, value_type(v));
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  template <typename T_Iterator>
  void _m_copy_initialize(size_type n, T_Iterator first) {
    for (size_type i = 0; i < n; ++i) {
      _m_construct_at(_m_start_ptr() + i, value_type(first[i]));
    }
    _m_data._m_finish += (n * sizeof(value_type));
  }

  template <typename T_Iterator>
  void _m_copy_initialize(T_Iterator first, T_Iterator last) {
    T_Iterator cur = first;
    size_type i = 0;
    for (; cur != last; ++i, ++cur) {
      _m_construct_at(_m_start_ptr() + i, value_type(*cur));
      _m_data._m_finish += sizeof(value_type);
    }
  }

  template <typename T_Iterator>
  value_type* _m_uninitialized_move(T_Iterator src_start, T_Iterator src_end, T_Iterator dst_start) {
    value_type* finish = std::addressof(*dst_start);
    for (T_Iterator cur = src_start; cur != src_end; ++cur, ++dst_start) {
      _m_construct_at(std::addressof(*dst_start), value_type(std::move(*cur)));
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
    pointer old_start = _m_start_ptr();
    pointer old_finish = _m_finish_ptr();
    auto [new_start, new_size] = _m_allocate(len);
    value_type* new_finish_ptr =
        _m_uninitialized_move(old_start, old_finish, alloc_traits::offset_to_pointer(new_start));
    _m_construct_at(new_finish_ptr, std::forward<T_Args>(args)...);
    new_finish_ptr++;
    _m_deallocate(old_start, capacity());
    _m_data._m_start = new_start;
    _m_data._m_finish = alloc_traits::pointer_to_offset(new_finish_ptr);
    _m_data._m_end_of_storage = new_start + new_size;
  }

  template <typename... T_Args>
  void _m_realloc_append_n(size_type n, T_Args&&... args) {
    const size_type len = _m_check_length(n, "ipcpp::vector::_m_realloc_append_n");
    pointer old_start = _m_start_ptr();
    pointer old_finish = _m_finish_ptr();
    auto [new_start, new_capacity] = _m_allocate(len);
    value_type* new_finish_ptr =
        _m_uninitialized_move(old_start, old_finish, alloc_traits::offset_to_pointer(new_start));
    for (size_type i = size(); i < n; ++i) {
      _m_construct_at(new_finish_ptr, std::forward<T_Args>(args)...);
      new_finish_ptr++;
    }
    _m_deallocate(old_start, capacity());
    _m_data._m_start = new_start;
    _m_data._m_finish = alloc_traits::pointer_to_offset(new_finish_ptr);
    _m_data._m_end_of_storage = new_start + new_capacity;
  }

  void _m_uninitialized_realloc_n(size_type n) {
    const size_type len = _m_check_length(n, "ipcpp::vector::_m_uninitialized_realloc_n");
    pointer old_start = _m_start_ptr();
    pointer old_finish = _m_finish_ptr();
    auto [new_start, new_size] = _m_allocate(len);
    value_type* new_finish_ptr =
        _m_uninitialized_move(old_start, old_finish, alloc_traits::offset_to_pointer(new_start));
    _m_deallocate(old_start, capacity());
    _m_data._m_start = new_start;
    _m_data._m_finish = alloc_traits::pointer_to_offset(new_finish_ptr);
    _m_data._m_end_of_storage = new_start + new_size;
  }

  void _m_realloc_copy(size_type n) {
    if (max_size() < n) {
      throw std::length_error(
          std::format("ipcpp::vector::_m_realloc_copy: length_error: max_size: {}, size: {}", max_size(), n));
    }
    pointer old_start = _m_start_ptr();
    pointer old_finish = _m_finish_ptr();
    auto [new_start, new_size] = _m_allocate(n);
    value_type* new_finish_ptr =
        _m_uninitialized_move(old_start, old_finish, alloc_traits::offset_to_pointer(new_start));
    _m_deallocate(old_start, capacity());
    _m_data._m_start = new_start;
    _m_data._m_finish = alloc_traits::pointer_to_offset(new_finish_ptr);
    _m_data._m_end_of_storage = new_start + new_size;
  }

  template <typename T_Iterator>
  void _m_move_to(T_Iterator first, T_Iterator last, T_Iterator dst) {
    if (dst == first) {
      return;
    } else if (dst < first || dst > last + 1) {
      for (; first < last; ++first) {
        _m_construct_at(std::addressof(*dst), *first);
        ++dst;
      }
    } else {
      difference_type offset = (last - first) - 1;
      for (; offset >= 0; --offset) {
        _m_construct_at(std::addressof(*(dst + offset)), *(first + offset));
      }
    }
  }

  template <typename... T_Args>
  void _m_construct_at(const_pointer addr, T_Args&&... args) {
    alloc_traits::construct(addr, std::forward<T_Args>(args)...);
  }

  template <typename T_Iterator>
  void _m_destroy(T_Iterator first, T_Iterator last) {
    for (; first != last; ++first) {
      alloc_traits::destroy(std::addressof(*first));
    }
  }

 private:  // ----------------------------------------------------------------------------------------------------------
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

template <class T_p, class T_Alloc>
bool operator==(const vector<T_p, T_Alloc>& lhs, const vector<T_p, T_Alloc>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (lhs.at(i) != rhs.at(i)) {
      return false;
    }
  }
  return true;
}

template <class T_p, class T_Alloc>
auto operator<=>(const vector<T_p, T_Alloc>& lhs,
                 const vector<T_p, T_Alloc>& rhs) -> decltype(lhs.front() <=> rhs.front()) {
  return std::lexicographical_compare_three_way(lhs.data(), lhs.data() + lhs.size(), rhs.data(),
                                                rhs.data() + rhs.size());
}

}  // namespace ipcpp