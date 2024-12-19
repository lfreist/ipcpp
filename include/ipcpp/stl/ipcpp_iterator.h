/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/concepts.h>
#include <ipcpp/utils/platform.h>

#include <compare>
#include <concepts>
#include <cstdint>
#include <iterator>

namespace ipcpp {

namespace detail {

template <typename T_p>
concept boolean_testable_impl = std::convertible_to<T_p, bool>;

template <typename T_p>
concept boolean_testable = boolean_testable_impl<T_p> && requires(T_p&& t) {
  { !static_cast<T_p&&>(t) } -> boolean_testable_impl;
};

template <typename T_p, typename U_p>
inline static constexpr bool S_noexcept(const T_p* t = nullptr, const U_p* u = nullptr) {
  if constexpr (std::three_way_comparable_with<T_p, U_p>)
    return noexcept(*t <=> *u);
  else
    return noexcept(*t < *u) && noexcept(*u < *t);
}

template <typename T_p, typename U_p>
[[nodiscard]]
inline constexpr auto synth3way(const T_p& t, const U_p& u) noexcept(S_noexcept<T_p, U_p>())
  requires requires {
    { t < u } -> boolean_testable;
    { u < t } -> boolean_testable;
  }
{
  if constexpr (std::three_way_comparable_with<T_p, U_p>)
    return t <=> u;
  else {
    if (t < u)
      return std::weak_ordering::less;
    else if (u < t)
      return std::weak_ordering::greater;
    else
      return std::weak_ordering::equivalent;
  }
}

template <typename T_p, typename U_p = T_p>
using synth3way_t = decltype(detail::synth3way(std::declval<T_p&>(), std::declval<U_p&>()));

}  // namespace detail

template <typename T_Iterator, typename T_Container>
class IPCPP_API normal_iterator {
 protected:
  T_Iterator _m_current;

  typedef std::iterator_traits<T_Iterator> traits_type;

  template <typename T_Iter>
  using convertible_from = std::enable_if_t<std::is_convertible_v<T_Iter, T_Iterator>>;

 public:
  typedef T_Iterator iterator_type;
  typedef typename traits_type::iterator_category iterator_category;
  typedef typename traits_type::value_type value_type;
  typedef typename traits_type::difference_type difference_type;
  typedef typename traits_type::reference reference;
  typedef typename traits_type::pointer pointer;

  constexpr normal_iterator() noexcept : _m_current(T_Iterator()) {}

  explicit constexpr normal_iterator(const T_Iterator& i) noexcept : _m_current(i) {}

  template <typename T_Iter, typename = convertible_from<T_Iter>>
  constexpr normal_iterator(const normal_iterator<T_Iter, T_Container>& i) noexcept : _m_current(i.base()) {}

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
constexpr detail::synth3way_t<T_IteratorL, T_IteratorR> operator<=>(
    const normal_iterator<T_IteratorL, T_Container>& lhs,
    const normal_iterator<T_IteratorR, T_Container>& rhs) noexcept(noexcept(detail::synth3way(lhs.base(),
                                                                                              rhs.base()))) {
  return detail::synth3way(lhs.base(), rhs.base());
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
constexpr detail::synth3way_t<T_Iterator> operator<=>(
    const normal_iterator<T_Iterator, T_Container>& lhs,
    const normal_iterator<T_Iterator, T_Container>& rhs) noexcept(noexcept(detail::synth3way(lhs.base(), rhs.base()))) {
  return detail::synth3way(lhs.base(), rhs.base());
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