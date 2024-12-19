/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <ipcpp/stl/allocator.h>

#include <memory>

namespace ipcpp {

template <typename T_Alloc>
struct allocator_traits {
  typedef T_Alloc allocator_type;
  typedef typename T_Alloc::value_type value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef typename T_Alloc::difference_type difference_type;
  typedef typename T_Alloc::size_type size_type;

 private:
  [[nodiscard]] static constexpr allocator_type get_allocator()
    requires requires {
      { allocator_type::get_singleton() } -> std::same_as<allocator_type>;
    }
  {
    return allocator_type::get_singleton();
  }

  [[nodiscard]] static constexpr allocator_type get_allocator()
    requires std::is_default_constructible_v<allocator_type>
  {
    return allocator_type();
  }

 public:
  [[nodiscard]] static constexpr difference_type allocate(size_type n) {
    if constexpr (requires(allocator_type a, size_type s) {
                    { a.allocate(s) } -> std::convertible_to<difference_type>;
                  }) {
      return {get_allocator().allocate(n), n * sizeof(value_type)};
    } else if constexpr (requires(allocator_type a, size_type s, const void* p) {
                           { a.allocate(s) } -> std::convertible_to<pointer>;
                         }) {
      return {pointer_to_offset(get_allocator().allocate(n)), n * sizeof(value_type)};
    } else {
      static_assert(false, "T_Alloc::allocate does not fulfill the requirements to be used with ipcpp::vector");
    }
  };

  [[nodiscard]] static constexpr std::pair<difference_type, size_type> allocate_at_least(size_type n) {
    if constexpr (requires(allocator_type a, size_type s) {
                    { a.allocate_at_least_offset(s) } -> std::convertible_to<std::pair<difference_type, size_type>>;
                  }) {
      return get_allocator().allocate_at_least_offset(n);
    } else if constexpr (requires(allocator_type a, size_type s) {
                           { a.allocate_at_least(s) } -> std::convertible_to<std::pair<difference_type, size_type>>;
                         }) {
      return get_allocator().allocate_at_least(n);
    } else if constexpr (requires(allocator_type a, size_type s) {
                           { a.allocate(s) } -> std::convertible_to<std::pair<difference_type, size_type>>;
                         }) {
      return get_allocator().allocate(n);
    } else if constexpr (requires(allocator_type a, size_type s) {
                           { a.allocate_offset(s) } -> std::convertible_to<std::pair<difference_type, size_type>>;
                         }) {
      return get_allocator().allocate_offset(n);
    } else if constexpr (requires(allocator_type a, size_type s) {
                           { a.allocate(s) } -> std::convertible_to<difference_type>;
                         }) {
      return {get_allocator().allocate(n), n * sizeof(value_type)};
    } else if constexpr (requires(allocator_type a, size_type s, const void* p) {
                           { a.allocate(s) } -> std::convertible_to<pointer>;
                         }) {
      return {pointer_to_offset(get_allocator().allocate(n)), n * sizeof(value_type)};
    } else {
      static_assert(false, "T_Alloc does not fulfill the requirements to be used with ipcpp::vector");
    }
  }

  static constexpr void deallocate(pointer p, size_type n) { get_allocator().deallocate(p, n); }

  static constexpr void deallocate_offset(difference_type o, size_type n) { deallocate(offset_to_pointer(o), n); }

  static constexpr pointer offset_to_pointer(difference_type offset) {
    if constexpr (requires(allocator_type a, difference_type d) {
                    { a.offset_to_pointer(d) } -> std::convertible_to<pointer>;
                  }) {
      return get_allocator().offset_to_pointer(offset);
    } else {
      if (offset < 0) {
        // vector stores offsets as std::ptrdiff initialized to -1
        return nullptr;
      }
      return reinterpret_cast<pointer>(offset);
    }
  }

  static constexpr difference_type pointer_to_offset(pointer p) {
    if constexpr (requires(allocator_type a, pointer ptr) {
                    { a.pointer_to_offset(ptr) } -> std::convertible_to<difference_type>;
                  }) {
      return get_allocator().pointer_to_offset(p);
    } else {
      if (p == nullptr) {
        return -1;
      }
      return reinterpret_cast<difference_type>(p);
    }
  }

  template <typename T_p, typename... T_Args>
    requires std::is_constructible_v<T_p, T_Args...>
  static constexpr void construct(T_p* p, T_Args&&... args) noexcept(std::is_nothrow_constructible_v<T_p, T_Args...>) {
#ifdef _MSC_VER
    ::new ((void*)p) T_p(std::forward<T_Args>(args)...);
#else
    std::construct_at(p, std::forward<T_Args>(args)...);
#endif
  }

  template <typename T_p>
  static constexpr void destroy(T_p* p) noexcept(std::is_nothrow_destructible_v<T_p>) {
    std::destroy_at(p);
  }

  static constexpr auto max_size() noexcept {
    if constexpr (requires(T_Alloc a) {
                    { a.max_size() } -> std::convertible_to<size_type>;
                  }) {
      return get_allocator().max_size();
    } else {
      return size_type(-1) / sizeof(value_type);
    }
  }
};

}  // namespace ipcpp