/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/concepts.h>

#include <compare>
#include <concepts>
#include <cstdint>
#include <iterator>

namespace ipcpp {

template <typename T_Iterator, typename T_Container>
class normal_iterator {
 protected:
  T_Iterator _m_current;

  typedef std::iterator_traits<T_Iterator> traits_type;

  template <typename T_Iter>
  using convertible_from = std::enable_if_t<std::is_convertible<T_Iter, T_Iterator>::value>;

 public:
  typedef T_Iterator iterator_type;
  typedef typename traits_type::iterator_category iterator_category;
  typedef typename traits_type::value_type value_type;
  typedef typename traits_type::difference_type difference_type;
  typedef typename traits_type::reference reference;
  typedef typename traits_type::pointer pointer;

  using iterator_concept = std::__detail::__iter_concept<T_Iterator>;

  constexpr normal_iterator() noexcept : _m_current(T_Iterator()) {}

  explicit constexpr normal_iterator(const T_Iterator& i) noexcept : _m_current(i) {}

  template <typename T_Iter, typename = convertible_from<T_Iter>>
  constexpr normal_iterator(const normal_iterator<T_Iter, T_Container>& i) noexcept
      : _m_current(i.base()) {}

  // Forward iterator requirements
  constexpr reference operator*() const noexcept { return *_m_current; }

  constexpr pointer operator->() const noexcept { return _m_current; }

  constexpr normal_iterator& operator++() noexcept {
    ++_m_current;
    return *this;
  }

  constexpr normal_iterator operator++(int) noexcept { return normal_iterator(_m_current++); }

  // Bidirectional iterator requirements
  constexpr normal_iterator& operator--() noexcept {
    --_m_current;
    return *this;
  }

  constexpr normal_iterator operator--(int) noexcept { return normal_iterator(_m_current--); }

  // Random access iterator requirements
  constexpr reference operator[](difference_type n) const noexcept { return _m_current[n]; }

  constexpr normal_iterator& operator+=(difference_type n) noexcept {
    _m_current += n;
    return *this;
  }

  constexpr normal_iterator operator+(difference_type n) const noexcept { return normal_iterator(_m_current + n); }

  constexpr normal_iterator& operator-=(difference_type n) noexcept {
    _m_current -= n;
    return *this;
  }

  constexpr normal_iterator operator-(difference_type n) const noexcept { return normal_iterator(_m_current - n); }

  constexpr const T_Iterator& base() const noexcept { return _m_current; }
};

template <typename T_IteratorL, typename T_IteratorR, typename T_Container>
[[nodiscard]]
constexpr bool operator==(const normal_iterator<T_IteratorL, T_Container>& lhs,
                          const normal_iterator<T_IteratorR, T_Container>& rhs) noexcept(noexcept(lhs.base() ==
                                                                                                  rhs.base()))
  requires requires {
    { lhs.base() == rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() == rhs.base();
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Container>
[[nodiscard]]
constexpr std::__detail::__synth3way_t<T_IteratorR, T_IteratorL> operator<=>(
    const normal_iterator<T_IteratorL, T_Container>& lhs,
    const normal_iterator<T_IteratorR, T_Container>& rhs) noexcept(noexcept(std::__detail::__synth3way(lhs.base(),
                                                                                                       rhs.base()))) {
  return std::__detail::__synth3way(lhs.base(), rhs.base());
}

template <typename T_Iterator, typename T_Container>
[[nodiscard]]
constexpr bool operator==(const normal_iterator<T_Iterator, T_Container>& lhs,
                          const normal_iterator<T_Iterator, T_Container>& rhs) noexcept(noexcept(lhs.base() ==
                                                                                                 rhs.base()))
  requires requires {
    { lhs.base() == rhs.base() } -> std::convertible_to<bool>;
  }
{
  return lhs.base() == rhs.base();
}

template <typename T_Iterator, typename T_Container>
[[nodiscard]]
constexpr std::__detail::__synth3way_t<T_Iterator> operator<=>(
    const normal_iterator<T_Iterator, T_Container>& lhs,
    const normal_iterator<T_Iterator, T_Container>& rhs) noexcept(noexcept(std::__detail::__synth3way(lhs.base(),
                                                                                                      rhs.base()))) {
  return std::__detail::__synth3way(lhs.base(), rhs.base());
}

template <typename T_IteratorL, typename T_IteratorR, typename T_Container>
[[nodiscard]] constexpr inline auto operator-(const normal_iterator<T_IteratorL, T_Container>& lhs,
                                                const normal_iterator<T_IteratorR, T_Container>& rhs) noexcept
    -> decltype(lhs.base() - rhs.base()) {
  return lhs.base() - rhs.base();
}

template <typename T_Iterator, typename T_Container>
[[nodiscard]] constexpr inline typename normal_iterator<T_Iterator, T_Container>::difference_type operator-(
    const normal_iterator<T_Iterator, T_Container>& lhs, const normal_iterator<T_Iterator, T_Container>& rhs) noexcept {
  return lhs.base() - rhs.base();
}

template <typename T_Iterator, typename T_Container>
[[nodiscard]] constexpr inline normal_iterator<T_Iterator, T_Container> operator+(
    typename normal_iterator<T_Iterator, T_Container>::difference_type n,
    const normal_iterator<T_Iterator, T_Container>& i) noexcept {
  return normal_iterator<T_Iterator, T_Container>(i.base() + n);
}



}  // namespace ipcpp