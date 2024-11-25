/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <ipcpp/container/concepts.h>

namespace ipcpp {

template <typename T_Iterator, typename T_Allocator>
class normal_iterator {
 public:
  typedef T_Iterator iterator_type;
  typedef T_Allocator allocator_type;
  typedef allocator_type::size_type size_type;
  typedef allocator_type::difference_type difference_type;
  typedef allocator_type::value_type value_type;
  typedef value_type& reference_type;
  typedef const value_type& const_reference_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static_assert(std::is_same_v<difference_type, iterator_type>,
                "iterator_type and allocator_type::difference_type must be the same");

 protected:
  allocator_type _m_allocator;
  difference_type _m_current;

 public:
  normal_iterator(difference_type v, const allocator_type& a) noexcept : _m_allocator(a), _m_current(v) {}
  normal_iterator(const normal_iterator& x) noexcept
      : _m_allocator(x._m_allocator), _m_current(x._m_current) {}

  normal_iterator& operator=(const normal_iterator&) = default;

  reference_type operator*() const noexcept { return *_m_allocator.addr_from_offset(_m_current); }

  pointer operator->() const noexcept { return _m_allocator.addr_from_offset(_m_current); }

  normal_iterator& operator++() noexcept {
    _m_current += sizeof(value_type);
    return *this;
  }

  normal_iterator operator++(int) noexcept {
    normal_iterator temp = *this;
    ++(*this);
    return temp;
  }

  normal_iterator& operator--() noexcept {
    _m_current -= sizeof(value_type);
    return *this;
  }

  normal_iterator operator--(int) noexcept {
    normal_iterator temp = *this;
    --(*this);
    return temp;
  }

  reference_type operator[](difference_type n) const noexcept {
    return _m_allocator.addr_from_offset(_m_current)[n];
  }

  normal_iterator& operator+=(difference_type n) noexcept {
    _m_current += (n * sizeof(value_type));
    return *this;
  }

  normal_iterator operator+(difference_type n) const noexcept {
    return normal_iterator(_m_current + (n * sizeof(value_type)), _m_allocator);
  }

  normal_iterator& operator-=(difference_type n) noexcept {
    _m_current -= (n * sizeof(value_type));
    return *this;
  }

  normal_iterator operator-(difference_type n) const noexcept {
    return normal_iterator(_m_current - (n * sizeof(value_type)), _m_allocator);
  }

  const iterator_type& base2() const noexcept { return _m_current; }
  pointer base() const noexcept { return _m_allocator.addr_from_offset(_m_current); }
};

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator==(
    const normal_iterator<T_IteratorL, T_Allocator>& lhs,
    const normal_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() == rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() == rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] std::strong_ordering operator<=>(
    const normal_iterator<T_IteratorL, T_Allocator>& lhs,
    const normal_iterator<T_IteratorR, T_Allocator>& rhs) noexcept {
  if constexpr (std::three_way_comparable_with<T_IteratorL, T_IteratorR>) {
    return lhs.base() <=> rhs.base();
  } else {
    if (lhs.base() < rhs.base()) return std::strong_ordering::less;
    if (lhs.base() > rhs.base()) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
  }
}

template <typename T_Iterator, typename T_Allocator>
[[nodiscard]] bool operator==(
    const normal_iterator<T_Iterator, T_Allocator>& lhs,
    const normal_iterator<T_Iterator, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() == rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() == rhs.base();
}

template <typename T_Iterator, typename T_Allocator>
[[nodiscard]] std::strong_ordering operator<=>(const normal_iterator<T_Iterator, T_Allocator>& lhs,
                                                         const normal_iterator<T_Iterator, T_Allocator>& rhs) noexcept {
  if constexpr (std::three_way_comparable<T_Iterator>) {
    return lhs.base() <=> rhs.base();
  } else {
    if (lhs < rhs) return std::strong_ordering::less;
    if (lhs > rhs) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
  }
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[__nodiscard__]] inline auto operator-(const normal_iterator<T_IteratorL, T_Allocator>& lhs,
                                                  const normal_iterator<T_IteratorR, T_Allocator>& rhs) noexcept
    -> decltype(lhs.base() - rhs.base()) {
  return lhs.base() - rhs.base();
}

template <typename T_Iterator, typename T_Allocator>
[[nodiscard]] inline typename normal_iterator<T_Iterator, T_Allocator>::difference_type
operator-(const normal_iterator<T_Iterator, T_Allocator>& lhs,
          const normal_iterator<T_Iterator, T_Allocator>& rhs) noexcept {
  return lhs.base() - rhs.base();
}

template<typename T_Iterator, typename T_Allocator>
[[nodiscard]]
    inline normal_iterator<T_Iterator, T_Allocator>
    operator+(typename normal_iterator<T_Iterator, T_Allocator>::difference_type
                  n, const normal_iterator<T_Iterator, T_Allocator>& i)
        noexcept
{ return normal_iterator<T_Iterator, T_Allocator>(i.base() + (n * sizeof(T_Iterator::value_type))); }

template <typename T_Iterator, typename T_Allocator>
class _reverse_iterator {
 public:
  typedef T_Iterator iterator_type;
  typedef T_Allocator allocator_type;
  typedef typename allocator_type::size_type size_type;
  typedef typename allocator_type::difference_type difference_type;
  typedef typename allocator_type::value_type value_type;
  typedef value_type& reference_type;
  typedef const value_type& const_reference_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static_assert(std::is_same_v<difference_type, iterator_type>,
                "iterator_type and allocator_type::difference_type must be the same");

 protected:
  allocator_type _m_allocator;
  difference_type _m_current;

 public:
  _reverse_iterator(difference_type v, const allocator_type& a) noexcept : _m_allocator(a), _m_current(v) {}
  _reverse_iterator(const _reverse_iterator& x) noexcept
      : _m_allocator(x._m_allocator), _m_current(x._m_current) {}

  _reverse_iterator& operator=(const _reverse_iterator&) = default;

  reference_type operator*() const noexcept { return *_m_allocator.addr_from_offset(_m_current); }

  pointer operator->() const noexcept { return _m_allocator.addr_from_offset(_m_current); }

  _reverse_iterator operator++(int) const noexcept {
    _reverse_iterator tmp = *this;
    _m_current -= sizeof(value_type);
    return tmp;
  }

  _reverse_iterator& operator++(int) noexcept {
    _m_current -= sizeof(value_type);
    return *this;
  }

  _reverse_iterator operator--(int) const noexcept {
    _reverse_iterator tmp = *this;
    _m_current += sizeof(value_type);
    return tmp;
  }

  _reverse_iterator& operator--(int) noexcept {
    _m_current += sizeof(value_type);
    return *this;
  }

  reference_type operator[](difference_type n) const noexcept {
    return *(_m_allocator.addr_from_offset(_m_current) - n - 1);
  }

  _reverse_iterator& operator+=(difference_type n) noexcept {
    _m_current -= (n * sizeof(value_type));
    return *this;
  }

  _reverse_iterator operator+(difference_type n) const noexcept {
    return _reverse_iterator(_m_current - (n * sizeof(value_type)));
  }

  _reverse_iterator& operator-=(difference_type n) noexcept {
    _m_current += (n * sizeof(value_type));
    return *this;
  }

  _reverse_iterator operator-(difference_type n) const noexcept {
    return _reverse_iterator(_m_current + (n * sizeof(value_type)));
  }

  const iterator_type& base() const noexcept { return _m_allocator.addr_from_offset(_m_current); }
};


template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator==(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() == rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() == rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator!=(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() != rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() != rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator<(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() > rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() > rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator>(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() < rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() < rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator<=(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() >= rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() >= rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Allocator>
[[nodiscard]] bool operator>=(
    const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
    const _reverse_iterator<T_IteratorR, T_Allocator>& rhs) noexcept(noexcept(lhs.base() == rhs.base()))
  requires requires {
    { lhs.base() <= rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() <= rhs.base();
}

template<typename T_IteratorL,
          std::three_way_comparable_with<T_IteratorL> T_IteratorR, typename T_Allocator>
[[nodiscard]]
std::compare_three_way_result_t<T_IteratorL, T_IteratorR>
operator<=>(const _reverse_iterator<T_IteratorL, T_Allocator>& lhs,
            const _reverse_iterator<T_IteratorR, T_Allocator>& rhs)
{ return lhs.base() <=> rhs.base(); }

}  // namespace ipcpp