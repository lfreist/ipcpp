/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <gtest/gtest.h>
#include <ipcpp/stl/vector.h>
#include <ipcpp/stl/allocator.h>

#include <list>
#include <algorithm>

constexpr static std::size_t allocator_mem_size = 8192;
static uint8_t alloc_mem[allocator_mem_size];

struct CustomType {
  CustomType(int x, double y) : a(x), b(y) {}
  int a;
  double b;
  bool operator==(const CustomType& other) const { return a == other.a; }
  bool operator<(const CustomType& other) const { return (a == other.a) ? b < other.b : a < other.a; }
  bool operator>(const CustomType& other) const { return (a == other.a) ? b > other.b : a > other.a; }
};

TEST(ipcpp_vector, default_constructor) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<double> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<std::string> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<CustomType> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    struct EmptyStruct {};

    carry::vector<EmptyStruct> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<std::unique_ptr<int>> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }
}

TEST(ipcpp_vector, constructor_size_value) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec(5, 42);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](int v) { return v == 42; }));
  }

  {
    carry::vector<double> vec(3, 3.14);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](double v) { return v == 3.14; }));
  }

  {
    carry::vector<std::string> vec(4, "test");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_GE(vec.capacity(), 4);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](const std::string& v) { return v == "test"; }));
  }

  {
    CustomType value{1, 2.5};
    carry::vector<CustomType> vec(2, value);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [&](const CustomType& v) { return v == value; }));
  }

  {
    struct EmptyStruct {};
    carry::vector<EmptyStruct> vec(6, EmptyStruct{});
    EXPECT_EQ(vec.size(), 6);
    EXPECT_GE(vec.capacity(), 6);
  }

  {
    carry::vector<int, std::allocator<int>> vec(6, 1);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_GE(vec.capacity(), 6);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [&](int i) { return i == 1; }));
  }
}

TEST(ipcpp_vector, copy_constructor) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> original{1, 2, 3, 4, 5};
    carry::vector<int> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    carry::vector<double> original{3.14, 2.71, 1.61};
    carry::vector<double> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    carry::vector<std::string> original{"one", "two", "three"};
    carry::vector<std::string> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    carry::vector<CustomType> original{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    carry::vector<void*> original{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    carry::vector<void*> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    // Cannot copy std::unique_ptr, so this tests that copy is unavailable
    static_assert(!std::is_copy_constructible_v<carry::vector<std::unique_ptr<int>>>);
  }

  {
    carry::vector<int, std::allocator<int>> original{1, 2, 3, 4, 5};
    carry::vector<int> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }

  {
    carry::vector<int> original{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_TRUE(std::equal(copy.begin(), copy.end(), original.begin()));
  }
}

TEST(ipcpp_vector, move_constructor) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> original{1, 2, 3, 4, 5};
    carry::vector<int> moved(std::move(original));
    EXPECT_EQ(moved.size(), 5);
    EXPECT_GE(moved.capacity(), 5);
    EXPECT_EQ(moved, (carry::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<double> original{3.14, 2.71, 1.61};
    carry::vector<double> moved(std::move(original));
    EXPECT_EQ(moved.size(), 3);
    EXPECT_GE(moved.capacity(), 3);
    EXPECT_EQ(moved, carry::vector<double>({3.14, 2.71, 1.61}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<std::string> original{"one", "two", "three"};
    carry::vector<std::string> moved(std::move(original));
    EXPECT_EQ(moved.size(), 3);
    EXPECT_GE(moved.capacity(), 3);
    EXPECT_EQ(moved, (carry::vector<std::string>{"one", "two", "three"}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<CustomType> original{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> moved(std::move(original));
    EXPECT_EQ(moved.size(), 2);
    EXPECT_GE(moved.capacity(), 2);
    EXPECT_EQ(moved, (carry::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<std::unique_ptr<int>> original;
    original.push_back(std::make_unique<int>(10));
    original.push_back(std::make_unique<int>(20));
    carry::vector<std::unique_ptr<int>> moved(std::move(original));
    EXPECT_EQ(moved.size(), 2);
    EXPECT_EQ(*moved[0], 10);
    EXPECT_EQ(*moved[1], 20);
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<int> original{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> moved(std::move(original));
    EXPECT_EQ(moved.size(), 5);
    EXPECT_GE(moved.capacity(), 5);
    EXPECT_EQ(moved, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }

  {
    carry::vector<int, std::allocator<int>> original{1, 2, 3, 4, 5};
    carry::vector<int> moved(std::move(original));
    EXPECT_EQ(moved.size(), 5);
    EXPECT_GE(moved.capacity(), 5);
    EXPECT_EQ(moved, (carry::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
  }
}

TEST(ipcpp_vector, constructor_initializer_list) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    carry::vector<double> vec{3.14, 2.71, 1.61};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<std::string>{"one", "two", "three"}));
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<void*>{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)}));
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5}));
  }
}

TEST(ipcpp_vector, constructor_iterator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    carry::vector<double> source{3.14, 2.71, 1.61};
    carry::vector<double> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    carry::vector<std::string> source{"one", "two", "three"};
    carry::vector<std::string> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    carry::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    carry::vector<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    carry::vector<void*> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<int> source{10, 20, 30, 40};
    carry::vector<int> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), source.begin()));
  }

  {
    carry::vector<int, std::allocator<int>> source{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_TRUE(std::equal(source.begin(), source.end(), vec.begin()));
  }

  {
    carry::vector<int, std::allocator<int>> source{1, 2, 3, 4, 5};
    carry::vector<int> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_TRUE(std::equal(source.begin(), source.end(), vec.begin()));
  }
}

struct Tracker {
  static inline int active_count = 0;
  Tracker() { ++active_count; }
  ~Tracker() { --active_count; }
};

TEST(ipcpp_vector, destructor) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<Tracker> vec(5);
    EXPECT_EQ(Tracker::active_count, 5);
  }

  EXPECT_EQ(Tracker::active_count, 0);

  {
    carry::vector<std::unique_ptr<int>> vec;
    vec.push_back(std::make_unique<int>(10));
    vec.push_back(std::make_unique<int>(20));
    EXPECT_EQ(vec.size(), 2);
  }

  EXPECT_EQ(Tracker::active_count, 0);

  {
    carry::vector<Tracker, std::allocator<Tracker>> vec(5);
    EXPECT_EQ(Tracker::active_count, 5);
  }

  EXPECT_EQ(Tracker::active_count, 0);

  {
    carry::vector<std::unique_ptr<int>, std::allocator<std::unique_ptr<int>>> vec;
    vec.push_back(std::make_unique<int>(10));
    vec.push_back(std::make_unique<int>(20));
    EXPECT_EQ(vec.size(), 2);
  }
}

TEST(ipcpp_vector, move_assign_operator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 5);
    EXPECT_GE(target.capacity(), 5);
    EXPECT_EQ(target, (carry::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    carry::vector<double> source{3.14, 2.71, 1.61};
    carry::vector<double> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 3);
    EXPECT_GE(target.capacity(), 3);
    EXPECT_EQ(target, (carry::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    carry::vector<std::string> source{"one", "two", "three"};
    carry::vector<std::string> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 3);
    EXPECT_GE(target.capacity(), 3);
    EXPECT_EQ(target, (carry::vector<std::string>{"one", "two", "three"}));
  }

  {
    carry::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 2);
    EXPECT_GE(target.capacity(), 2);
    EXPECT_EQ(target, (carry::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    carry::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(10));
    source.push_back(std::make_unique<int>(20));

    carry::vector<std::unique_ptr<int>> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(*target[0], 10);
    EXPECT_EQ(*target[1], 20);
  }
}

TEST(ipcpp_vector, copy_assign_operator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int> target;
    target = source;

    EXPECT_EQ(source.size(), 5);
    EXPECT_EQ(target.size(), 5);
    EXPECT_EQ(source, target);
  }

  {
    carry::vector<double> source{3.14, 2.71, 1.61};
    carry::vector<double> target;
    target = source;

    EXPECT_EQ(source.size(), 3);
    EXPECT_EQ(target.size(), 3);
    EXPECT_EQ(source, target);
  }

  {
    carry::vector<std::string> source{"one", "two", "three"};
    carry::vector<std::string> target;
    target = source;

    EXPECT_EQ(source.size(), 3);
    EXPECT_EQ(target.size(), 3);
    EXPECT_EQ(source, target);
  }

  {
    carry::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> target;
    target = source;

    EXPECT_EQ(source.size(), 2);
    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(source, target);
  }

  {
    carry::vector<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    carry::vector<void*> target;
    target = source;

    EXPECT_EQ(source.size(), 2);
    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(source, target);
  }

  {
    carry::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    source = source;

    EXPECT_EQ(source.size(), 2);
  }

  {
    carry::vector<int, std::allocator<int>> source{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> target;
    target = std::move(source);

    EXPECT_EQ(target.size(), 5);
    EXPECT_GE(target.capacity(), 5);
    EXPECT_EQ(target, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5}));
  }
}

TEST(ipcpp_vector, ilist_assign_operator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    vec = {1, 2, 3, 4, 5};

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    carry::vector<double> vec{1, 2, 3, 4};
    vec = {3.14, 2.71, 1.61};

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    carry::vector<std::string> vec;
    vec = {"one", "two", "three"};

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<std::string>{"one", "two", "three"}));
  }

  {
    carry::vector<CustomType> vec;
    vec = {{1, 2.5}, {3, 4.5}};

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    carry::vector<void*> vec;
    vec = {reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<void*>{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)}));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec = {1, 2, 3, 4, 5};

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5}));
  }
}

TEST(ipcpp_vector, assign_size_value) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    vec.assign(5, 42);

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](int x) { return x == 42; }));
  }

  {
    carry::vector<double> vec;
    vec.assign(3, 3.14);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](double x) { return x == 3.14; }));
  }

  {
    carry::vector<std::string> vec;
    vec.assign(4, "test");

    EXPECT_EQ(vec.size(), 4);
    EXPECT_GE(vec.capacity(), 4);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](const std::string& s) { return s == "test"; }));
  }

  {
    carry::vector<CustomType> vec;
    vec.assign(2, {42, 3.14});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_TRUE(
        std::all_of(vec.begin(), vec.end(), [](const CustomType& obj) { return obj.a == 42 && obj.b == 3.14; }));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec.assign(5, 42);

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_TRUE(std::all_of(vec.begin(), vec.end(), [](int x) { return x == 42; }));
  }
}

TEST(ipcpp_vector, assign_iterator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<double> source{3.14, 2.71, 1.61};
    carry::vector<double> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), source.begin()));
  }

  {
    std::list<std::string> source{"one", "two", "three"};
    carry::vector<std::string> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), source.begin()));
  }

  {
    carry::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    carry::vector<CustomType> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    carry::vector<void*> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), source.begin()));
  }

  {
    carry::vector<int, std::allocator<int>> source{1, 2, 3, 4, 5};
    carry::vector<int> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(source.begin(), source.end(), vec.begin()));
  }

  {
    carry::vector<int> source{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(source.begin(), source.end(), vec.begin()));
  }

  {
    carry::vector<int, std::allocator<int>> source{1, 2, 3, 4, 5};
    carry::vector<int, std::allocator<int>> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_EQ(vec, source);
  }
}

TEST(ipcpp_vector, assign_initializer_list) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    vec.assign({1, 2, 3, 4, 5});

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    carry::vector<double> vec;
    vec.assign({3.14, 2.71, 1.61});

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    carry::vector<std::string> vec;
    vec.assign({"one", "two", "three"});

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (carry::vector<std::string>{"one", "two", "three"}));
  }

  {
    carry::vector<CustomType> vec;
    vec.assign({{1, 2.5}, {3, 4.5}});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    carry::vector<void*> vec;
    vec.assign({reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (carry::vector<void*>{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)}));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec.assign({1, 2, 3, 4, 5});

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5}));
  }
}

TEST(ipcpp_vector, operator_at) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 30);
    EXPECT_EQ(vec[3], 40);
    EXPECT_EQ(vec[4], 50);

    vec[2] = 100;
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 100);
    EXPECT_EQ(vec[3], 40);
    EXPECT_EQ(vec[4], 50);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4, 5.5};
    EXPECT_EQ(vec[0], 1.1);
    EXPECT_EQ(vec[1], 2.2);
    EXPECT_EQ(vec[2], 3.3);
    EXPECT_EQ(vec[3], 4.4);
    EXPECT_EQ(vec[4], 5.5);

    vec[4] = 9.9;
    EXPECT_EQ(vec[0], 1.1);
    EXPECT_EQ(vec[1], 2.2);
    EXPECT_EQ(vec[2], 3.3);
    EXPECT_EQ(vec[3], 4.4);
    EXPECT_EQ(vec[4], 9.9);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three", "four", "five"};
    EXPECT_EQ(vec[0], "one");
    EXPECT_EQ(vec[1], "two");
    EXPECT_EQ(vec[2], "three");
    EXPECT_EQ(vec[3], "four");
    EXPECT_EQ(vec[4], "five");

    vec[1] = "updated";
    EXPECT_EQ(vec[0], "one");
    EXPECT_EQ(vec[1], "updated");
    EXPECT_EQ(vec[2], "three");
    EXPECT_EQ(vec[3], "four");
    EXPECT_EQ(vec[4], "five");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    EXPECT_EQ(vec[0], (CustomType{1, 2.5}));
    EXPECT_EQ(vec[1], (CustomType{3, 4.5}));
    EXPECT_EQ(vec[2], (CustomType{5, 6.5}));

    vec[1] = {7, 8.9};
    EXPECT_EQ(vec[0], (CustomType{1, 2.5}));
    EXPECT_EQ(vec[1], (CustomType{7, 8.9}));
    EXPECT_EQ(vec[2], (CustomType{5, 6.5}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2), reinterpret_cast<void*>(0x3)};
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec[1], reinterpret_cast<void*>(0x2));
    EXPECT_EQ(vec[2], reinterpret_cast<void*>(0x3));

    vec[1] = reinterpret_cast<void*>(0x4);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec[1], reinterpret_cast<void*>(0x4));
    EXPECT_EQ(vec[2], reinterpret_cast<void*>(0x3));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 30);
    EXPECT_EQ(vec[3], 40);
    EXPECT_EQ(vec[4], 50);

    vec[2] = 100;
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 100);
    EXPECT_EQ(vec[3], 40);
    EXPECT_EQ(vec[4], 50);
  }
}

TEST(ipcpp_vector, at) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);

    vec.at(2) = 100;
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 100);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);

    EXPECT_THROW(vec.at(5), std::out_of_range);
    EXPECT_THROW(vec.at(-1), std::out_of_range);

    vec.push_back(1);
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 100);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);
    EXPECT_EQ(vec.at(5), 1);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4, 5.5};
    EXPECT_EQ(vec.at(0), 1.1);
    EXPECT_EQ(vec.at(1), 2.2);
    EXPECT_EQ(vec.at(2), 3.3);
    EXPECT_EQ(vec.at(3), 4.4);
    EXPECT_EQ(vec.at(4), 5.5);

    vec.at(4) = 9.9;
    EXPECT_EQ(vec.at(0), 1.1);
    EXPECT_EQ(vec.at(1), 2.2);
    EXPECT_EQ(vec.at(2), 3.3);
    EXPECT_EQ(vec.at(3), 4.4);
    EXPECT_EQ(vec.at(4), 9.9);

    EXPECT_THROW(vec.at(6), std::out_of_range);

    vec.push_back(0.0);

    EXPECT_EQ(vec.at(0), 1.1);
    EXPECT_EQ(vec.at(1), 2.2);
    EXPECT_EQ(vec.at(2), 3.3);
    EXPECT_EQ(vec.at(3), 4.4);
    EXPECT_EQ(vec.at(4), 9.9);
    EXPECT_EQ(vec.at(5), 0.0);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three", "four", "five"};
    EXPECT_EQ(vec.at(0), "one");
    EXPECT_EQ(vec.at(1), "two");
    EXPECT_EQ(vec.at(2), "three");
    EXPECT_EQ(vec.at(3), "four");
    EXPECT_EQ(vec.at(4), "five");

    vec.at(1) = "updated";
    EXPECT_EQ(vec.at(0), "one");
    EXPECT_EQ(vec.at(1), "updated");
    EXPECT_EQ(vec.at(2), "three");
    EXPECT_EQ(vec.at(3), "four");
    EXPECT_EQ(vec.at(4), "five");

    EXPECT_THROW(vec.at(10), std::out_of_range);

    vec.push_back("back");
    EXPECT_EQ(vec.at(0), "one");
    EXPECT_EQ(vec.at(1), "updated");
    EXPECT_EQ(vec.at(2), "three");
    EXPECT_EQ(vec.at(3), "four");
    EXPECT_EQ(vec.at(4), "five");
    EXPECT_EQ(vec.at(5), "back");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    EXPECT_EQ(vec.at(0), (CustomType{1, 2.5}));
    EXPECT_EQ(vec.at(1), (CustomType{3, 4.5}));
    EXPECT_EQ(vec.at(2), (CustomType{5, 6.5}));

    vec.at(1) = {7, 8.9};
    EXPECT_EQ(vec.at(0), (CustomType{1, 2.5}));
    EXPECT_EQ(vec.at(1), (CustomType{7, 8.9}));
    EXPECT_EQ(vec.at(2), (CustomType{5, 6.5}));

    EXPECT_THROW(vec.at(3), std::out_of_range);

    vec.push_back({10, 4.3});
    EXPECT_EQ(vec.at(0), (CustomType{1, 2.5}));
    EXPECT_EQ(vec.at(1), (CustomType{7, 8.9}));
    EXPECT_EQ(vec.at(2), (CustomType{5, 6.5}));
    EXPECT_EQ(vec.at(3), (CustomType{10, 4.3}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2), reinterpret_cast<void*>(0x3)};
    EXPECT_EQ(vec.at(0), reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec.at(1), reinterpret_cast<void*>(0x2));
    EXPECT_EQ(vec.at(2), reinterpret_cast<void*>(0x3));

    vec.at(1) = reinterpret_cast<void*>(0x4);
    EXPECT_EQ(vec.at(0), reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec.at(1), reinterpret_cast<void*>(0x4));
    EXPECT_EQ(vec.at(2), reinterpret_cast<void*>(0x3));

    EXPECT_THROW(vec.at(4), std::out_of_range);

    vec.push_back(reinterpret_cast<void*>(0x5));
    EXPECT_EQ(vec.at(0), reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec.at(1), reinterpret_cast<void*>(0x4));
    EXPECT_EQ(vec.at(2), reinterpret_cast<void*>(0x3));
    EXPECT_EQ(vec.at(3), reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);

    vec.at(2) = 100;
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 100);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);

    EXPECT_THROW(vec.at(5), std::out_of_range);
    EXPECT_THROW(vec.at(-1), std::out_of_range);

    vec.push_back(1);
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 100);
    EXPECT_EQ(vec.at(3), 40);
    EXPECT_EQ(vec.at(4), 50);
    EXPECT_EQ(vec.at(5), 1);
  }
}

TEST(ipcpp_vector, front) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.front(), 10);

    vec.front() = 99;
    EXPECT_EQ(vec.front(), 99);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};
    EXPECT_EQ(vec.front(), 1.1);

    vec.front() = 9.9;
    EXPECT_EQ(vec.front(), 9.9);
  }

  {
    carry::vector<std::string> vec{"first", "second", "third"};
    EXPECT_EQ(vec.front(), "first");

    vec.front() = "updated";
    EXPECT_EQ(vec.front(), "updated");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    EXPECT_EQ(vec.front(), (CustomType{1, 2.5}));

    vec.front() = {7, 8.9};
    EXPECT_EQ(vec.front(), (CustomType{7, 8.9}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    EXPECT_EQ(vec.front(), reinterpret_cast<void*>(0x1));

    vec.front() = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec.front(), reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.front(), 10);

    vec.front() = 99;
    EXPECT_EQ(vec.front(), 99);
  }
}

TEST(ipcpp_vector, back) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.back(), 50);

    vec.back() = 99;
    EXPECT_EQ(vec.back(), 99);

    vec.push_back(100);
    EXPECT_EQ(vec.back(), 100);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};
    EXPECT_EQ(vec.back(), 3.3);

    vec.back() = 9.9;
    EXPECT_EQ(vec.back(), 9.9);

    vec.push_back(0.0);
    EXPECT_EQ(vec.back(), 0.0);
  }

  {
    carry::vector<std::string> vec{"first", "second", "third"};
    EXPECT_EQ(vec.back(), "third");

    vec.back() = "updated";
    EXPECT_EQ(vec.back(), "updated");

    vec.push_back("hello");
    EXPECT_EQ(vec.back(), "hello");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    EXPECT_EQ(vec.back(), (CustomType{5, 6.5}));

    vec.back() = {7, 8.9};
    EXPECT_EQ(vec.back(), (CustomType{7, 8.9}));

    vec.push_back({1, 1.0});
    EXPECT_EQ(vec.back(), (CustomType{1, 1.0}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    EXPECT_EQ(vec.back(), reinterpret_cast<void*>(0x2));

    vec.back() = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec.back(), reinterpret_cast<void*>(0x5));

    vec.push_back(reinterpret_cast<void*>(0xA));
    EXPECT_EQ(vec.back(), reinterpret_cast<void*>(0xA));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    EXPECT_EQ(vec.back(), 50);

    vec.back() = 99;
    EXPECT_EQ(vec.back(), 99);

    vec.push_back(100);
    EXPECT_EQ(vec.back(), 100);
  }
}

TEST(ipcpp_vector, data) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    int* data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], 10);
    EXPECT_EQ(data_ptr[1], 20);
    EXPECT_EQ(data_ptr[2], 30);
    EXPECT_EQ(data_ptr[3], 40);
    EXPECT_EQ(data_ptr[4], 50);

    data_ptr[2] = 100;
    EXPECT_EQ(vec[2], 100);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    double* data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], 1.1);
    EXPECT_EQ(data_ptr[1], 2.2);
    EXPECT_EQ(data_ptr[2], 3.3);
    EXPECT_EQ(data_ptr[3], 4.4);

    data_ptr[3] = 9.9;
    EXPECT_EQ(vec[3], 9.9);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    std::string* data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], "one");
    EXPECT_EQ(data_ptr[1], "two");
    EXPECT_EQ(data_ptr[2], "three");

    data_ptr[1] = "updated";
    EXPECT_EQ(vec[1], "updated");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    CustomType* data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], (CustomType{1, 2.5}));
    EXPECT_EQ(data_ptr[1], (CustomType{3, 4.5}));
    EXPECT_EQ(data_ptr[2], (CustomType{5, 6.5}));

    data_ptr[2] = {7, 8.9};
    EXPECT_EQ(vec[2], (CustomType{7, 8.9}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    void** data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], reinterpret_cast<void*>(0x1));
    EXPECT_EQ(data_ptr[1], reinterpret_cast<void*>(0x2));

    data_ptr[1] = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec[1], reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    int* data_ptr = vec.data();

    EXPECT_EQ(data_ptr[0], 10);
    EXPECT_EQ(data_ptr[1], 20);
    EXPECT_EQ(data_ptr[2], 30);
    EXPECT_EQ(data_ptr[3], 40);
    EXPECT_EQ(data_ptr[4], 50);

    data_ptr[2] = 100;
    EXPECT_EQ(vec[2], 100);
  }
}

TEST(ipcpp_vector, begin) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    auto it = vec.begin();

    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);

    *it = 100;
    EXPECT_EQ(vec[2], 100);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto it = vec.begin();

    EXPECT_EQ(*it, 1.1);
    ++it;
    EXPECT_EQ(*it, 2.2);
    ++it;
    EXPECT_EQ(*it, 3.3);

    *it = 9.9;
    EXPECT_EQ(vec[2], 9.9);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    auto it = vec.begin();

    EXPECT_EQ(*it, "one");
    ++it;
    EXPECT_EQ(*it, "two");
    ++it;
    EXPECT_EQ(*it, "three");

    *it = "updated";
    EXPECT_EQ(vec[2], "updated");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto it = vec.begin();

    EXPECT_EQ(*it, (CustomType{1, 2.5}));
    ++it;
    EXPECT_EQ(*it, (CustomType{3, 4.5}));
    ++it;
    EXPECT_EQ(*it, (CustomType{5, 6.5}));

    *it = {7, 8.9};
    EXPECT_EQ(vec[2], (CustomType{7, 8.9}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto it = vec.begin();

    EXPECT_EQ(*it, reinterpret_cast<void*>(0x1));
    ++it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x2));

    *it = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec[1], reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto it = vec.begin();

    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);

    *it = 100;
    EXPECT_EQ(vec[2], 100);
  }
}

TEST(ipcpp_vector, cbegin) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    const carry::vector<int> vec{10, 20, 30, 40, 50};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);
  }

  {
    const carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, 1.1);
    ++it;
    EXPECT_EQ(*it, 2.2);
    ++it;
    EXPECT_EQ(*it, 3.3);
  }

  {
    const carry::vector<std::string> vec{"one", "two", "three"};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, "one");
    ++it;
    EXPECT_EQ(*it, "two");
    ++it;
    EXPECT_EQ(*it, "three");
  }

  {
    const carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, (CustomType{1, 2.5}));
    ++it;
    EXPECT_EQ(*it, (CustomType{3, 4.5}));
    ++it;
    EXPECT_EQ(*it, (CustomType{5, 6.5}));
  }

  {
    const carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, reinterpret_cast<void*>(0x1));
    ++it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x2));
  }

  {
    const carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto it = vec.cbegin();

    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
    ++it;
    EXPECT_EQ(*it, 30);
  }
}

TEST(ipcpp_vector, rbegin) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, 50);
    ++rit;
    EXPECT_EQ(*rit, 40);
    ++rit;
    EXPECT_EQ(*rit, 30);

    *rit = 99;
    EXPECT_EQ(vec[2], 99);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, 4.4);
    ++rit;
    EXPECT_EQ(*rit, 3.3);
    ++rit;
    EXPECT_EQ(*rit, 2.2);

    *rit = 8.8;
    EXPECT_EQ(vec[1], 8.8);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, "three");
    ++rit;
    EXPECT_EQ(*rit, "two");
    ++rit;
    EXPECT_EQ(*rit, "one");

    *rit = "updated";
    EXPECT_EQ(vec[0], "updated");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, (CustomType{5, 6.5}));
    ++rit;
    EXPECT_EQ(*rit, (CustomType{3, 4.5}));
    ++rit;
    EXPECT_EQ(*rit, (CustomType{1, 2.5}));

    *rit = {7, 8.9};
    EXPECT_EQ(vec[0], (CustomType{7, 8.9}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x2));
    ++rit;
    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x1));

    *rit = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto rit = vec.rbegin();

    EXPECT_EQ(*rit, 50);
    ++rit;
    EXPECT_EQ(*rit, 40);
    ++rit;
    EXPECT_EQ(*rit, 30);

    *rit = 99;
    EXPECT_EQ(vec[2], 99);
  }
}

TEST(ipcpp_vector, end) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{10, 20, 30, 40, 50};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, 50);
    --it;
    EXPECT_EQ(*it, 40);
    --it;
    EXPECT_EQ(*it, 30);

    *it = 99;
    EXPECT_EQ(vec[2], 99);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, 4.4);
    --it;
    EXPECT_EQ(*it, 3.3);
    --it;
    EXPECT_EQ(*it, 2.2);

    *it = 8.8;
    EXPECT_EQ(vec[1], 8.8);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, "three");
    --it;
    EXPECT_EQ(*it, "two");
    --it;
    EXPECT_EQ(*it, "one");

    *it = "updated";
    EXPECT_EQ(vec[0], "updated");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, (CustomType{5, 6.5}));
    --it;
    EXPECT_EQ(*it, (CustomType{3, 4.5}));
    --it;
    EXPECT_EQ(*it, (CustomType{1, 2.5}));

    *it = {7, 8.9};
    EXPECT_EQ(vec[0], (CustomType{7, 8.9}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x2));
    --it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x1));

    *it = reinterpret_cast<void*>(0x5);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x5));
  }

  {
    carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto it = vec.end();

    --it;
    EXPECT_EQ(*it, 50);
    --it;
    EXPECT_EQ(*it, 40);
    --it;
    EXPECT_EQ(*it, 30);

    *it = 99;
    EXPECT_EQ(vec[2], 99);
  }
}

TEST(ipcpp_vector, cend) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    const carry::vector<int> vec{10, 20, 30, 40, 50};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, 50);
    --it;
    EXPECT_EQ(*it, 40);
    --it;
    EXPECT_EQ(*it, 30);
  }

  {
    const carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, 4.4);
    --it;
    EXPECT_EQ(*it, 3.3);
    --it;
    EXPECT_EQ(*it, 2.2);
  }

  {
    const carry::vector<std::string> vec{"one", "two", "three"};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, "three");
    --it;
    EXPECT_EQ(*it, "two");
    --it;
    EXPECT_EQ(*it, "one");
  }

  {
    const carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, (CustomType{5, 6.5}));
    --it;
    EXPECT_EQ(*it, (CustomType{3, 4.5}));
    --it;
    EXPECT_EQ(*it, (CustomType{1, 2.5}));
  }

  {
    const carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x2));
    --it;
    EXPECT_EQ(*it, reinterpret_cast<void*>(0x1));
  }

  {
    const carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto it = vec.cend();

    --it;
    EXPECT_EQ(*it, 50);
    --it;
    EXPECT_EQ(*it, 40);
    --it;
    EXPECT_EQ(*it, 30);
  }
}

TEST(ipcpp_vector, crbegin) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    const carry::vector<int> vec{10, 20, 30, 40, 50};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, 50);
    ++rit;
    EXPECT_EQ(*rit, 40);
    ++rit;
    EXPECT_EQ(*rit, 30);
  }

  {
    const carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, 4.4);
    ++rit;
    EXPECT_EQ(*rit, 3.3);
    ++rit;
    EXPECT_EQ(*rit, 2.2);
  }

  {
    const carry::vector<std::string> vec{"one", "two", "three"};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, "three");
    ++rit;
    EXPECT_EQ(*rit, "two");
    ++rit;
    EXPECT_EQ(*rit, "one");
  }

  {
    const carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, (CustomType{5, 6.5}));
    ++rit;
    EXPECT_EQ(*rit, (CustomType{3, 4.5}));
    ++rit;
    EXPECT_EQ(*rit, (CustomType{1, 2.5}));
  }

  {
    const carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x2));
    ++rit;
    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x1));
  }

  {
    const carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto rit = vec.crbegin();

    EXPECT_EQ(*rit, 50);
    ++rit;
    EXPECT_EQ(*rit, 40);
    ++rit;
    EXPECT_EQ(*rit, 30);
  }
}

TEST(ipcpp_vector, crend) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    const carry::vector<int> vec{10, 20, 30, 40, 50};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, 10);
    --rit;
    EXPECT_EQ(*rit, 20);
    --rit;
    EXPECT_EQ(*rit, 30);
  }

  {
    const carry::vector<double> vec{1.1, 2.2, 3.3, 4.4};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, 1.1);
    --rit;
    EXPECT_EQ(*rit, 2.2);
    --rit;
    EXPECT_EQ(*rit, 3.3);
  }

  {
    const carry::vector<std::string> vec{"one", "two", "three"};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, "one");
    --rit;
    EXPECT_EQ(*rit, "two");
    --rit;
    EXPECT_EQ(*rit, "three");
  }

  {
    const carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, (CustomType{1, 2.5}));
    --rit;
    EXPECT_EQ(*rit, (CustomType{3, 4.5}));
    --rit;
    EXPECT_EQ(*rit, (CustomType{5, 6.5}));
  }

  {
    const carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x1));
    --rit;
    EXPECT_EQ(*rit, reinterpret_cast<void*>(0x2));
  }

  {
    const carry::vector<int, std::allocator<int>> vec{10, 20, 30, 40, 50};
    auto rit = vec.crend();

    --rit;
    EXPECT_EQ(*rit, 10);
    --rit;
    EXPECT_EQ(*rit, 20);
    --rit;
    EXPECT_EQ(*rit, 30);
  }
}

TEST(ipcpp_vector, empty) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    EXPECT_TRUE(vec.empty());

    vec.push_back(10);
    EXPECT_FALSE(vec.empty());

    vec.clear();
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<double> vec{1.1, 2.2};
    EXPECT_FALSE(vec.empty());

    vec.pop_back();
    vec.pop_back();
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<std::string> vec{"one"};
    EXPECT_FALSE(vec.empty());

    vec.erase(vec.begin());
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}};
    EXPECT_FALSE(vec.empty());

    vec.pop_back();
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<void*> vec;
    EXPECT_TRUE(vec.empty());

    vec.push_back(reinterpret_cast<void*>(0x1));
    EXPECT_FALSE(vec.empty());
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    EXPECT_TRUE(vec.empty());

    vec.push_back(10);
    EXPECT_FALSE(vec.empty());

    vec.clear();
    EXPECT_TRUE(vec.empty());
  }
}

TEST(ipcpp_vector, size) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    EXPECT_EQ(vec.size(), 0);

    vec.push_back(10);
    EXPECT_EQ(vec.size(), 1);

    vec.push_back(20);
    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};
    EXPECT_EQ(vec.size(), 3);

    vec.push_back(4.4);
    EXPECT_EQ(vec.size(), 4);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 3);
  }

  {
    carry::vector<std::string> vec{"one", "two"};
    EXPECT_EQ(vec.size(), 2);

    vec.push_back("three");
    EXPECT_EQ(vec.size(), 3);

    vec.erase(vec.begin());
    EXPECT_EQ(vec.size(), 2);
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.size(), 2);

    vec.push_back({5, 6.5});
    EXPECT_EQ(vec.size(), 3);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 2);
  }

  {
    carry::vector<void*> vec;
    EXPECT_EQ(vec.size(), 0);

    vec.push_back(reinterpret_cast<void*>(0x1));
    EXPECT_EQ(vec.size(), 1);

    vec.push_back(reinterpret_cast<void*>(0x2));
    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    EXPECT_EQ(vec.size(), 0);

    vec.push_back(10);
    EXPECT_EQ(vec.size(), 1);

    vec.push_back(20);
    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
  }
}

TEST(ipcpp_vector, max_size) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    EXPECT_EQ(vec.max_size(), carry::pool_allocator<int>::get_singleton().max_size());
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};
    EXPECT_EQ(vec.max_size(), carry::pool_allocator<double>::get_singleton().max_size());
  }

  {
    carry::vector<std::string> vec{"one", "two"};
    EXPECT_EQ(vec.max_size(), carry::pool_allocator<std::string>::get_singleton().max_size());
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.max_size(), carry::pool_allocator<CustomType>::get_singleton().max_size());
  }

  {
    carry::vector<void*> vec;
    EXPECT_EQ(vec.max_size(), carry::pool_allocator<void*>::get_singleton().max_size());
  }
}

TEST(ipcpp_vector, reserve_capacity) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    EXPECT_EQ(vec.capacity(), 0);

    vec.reserve(5);
    EXPECT_GE(vec.capacity(), 5);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.reserve(5);
    EXPECT_GE(vec.capacity(), 10);
  }

  {
    carry::vector<double> vec{1.1, 2.2};
    EXPECT_EQ(vec.capacity(), 2);

    vec.reserve(4);
    EXPECT_GE(vec.capacity(), 4);

    vec.reserve(8);
    EXPECT_GE(vec.capacity(), 8);

    vec.reserve(3);
    EXPECT_GE(vec.capacity(), 8);
  }

  {
    carry::vector<std::string> vec{"one", "two"};
    EXPECT_EQ(vec.capacity(), 2);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.reserve(5);
    EXPECT_GE(vec.capacity(), 10);
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.capacity(), 2);

    vec.reserve(6);
    EXPECT_GE(vec.capacity(), 6);

    vec.reserve(4);
    EXPECT_GE(vec.capacity(), 6);
  }

  {
    carry::vector<void*> vec;
    EXPECT_EQ(vec.capacity(), 0);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.reserve(20);
    EXPECT_GE(vec.capacity(), 20);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 20);
  }

  // Edge case testing with reserve(0)
  {
    carry::vector<int> vec;
    vec.reserve(0);
    EXPECT_GE(vec.capacity(), 0);
  }

  {
    carry::vector<int> vec;
    EXPECT_THROW(vec.reserve(vec.max_size() + 1), std::length_error);
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    EXPECT_EQ(vec.capacity(), 0);

    vec.reserve(5);
    EXPECT_GE(vec.capacity(), 5);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.reserve(5);
    EXPECT_GE(vec.capacity(), 10);
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    EXPECT_THROW(vec.reserve(vec.max_size() + 1), std::length_error);
  }
}

TEST(ipcpp_vector, shrink_to_fit) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec;
    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<int> vec{1, 2, 3, 4, 5};
    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);

    vec.shrink_to_fit();
    EXPECT_LT(vec.capacity(), 100);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);

    vec.shrink_to_fit();
    EXPECT_LT(vec.capacity(), 100);
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};
    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);

    vec.shrink_to_fit();
    EXPECT_LT(vec.capacity(), 100);
  }

  {
    carry::vector<void*> vec;
    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<int> vec{1, 2, 3};
    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);

    vec.shrink_to_fit();
    EXPECT_LT(vec.capacity(), 100);
  }

  {
    carry::vector<int> vec;
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
  }
}

TEST(ipcpp_vector, clear) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};
    EXPECT_EQ(vec.size(), 5);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};
    EXPECT_EQ(vec.size(), 3);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};
    EXPECT_EQ(vec.size(), 3);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    EXPECT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  // Case with an already empty vector
  {
    carry::vector<int> vec;
    EXPECT_EQ(vec.size(), 0);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  // Clear vector with large number of elements
  {
    carry::vector<int> vec(1000, 1);
    EXPECT_EQ(vec.size(), 1000);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};
    EXPECT_EQ(vec.size(), 5);

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }
}

TEST(ipcpp_vector, insert_const_iterator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    vec.insert(vec.cbegin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};

    vec.insert(vec.cbegin() + 1, 4.4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[1], 4.4);
    EXPECT_EQ(vec[2], 2.2);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};

    std::string i("zero");
    vec.insert(vec.cbegin(), i);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "zero");
    EXPECT_EQ(vec[1], "one");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};

    vec.insert(vec.cbegin() + 1, {7, 8.5});
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[1], (CustomType{7, 8.5}));
    EXPECT_EQ(vec[2], (CustomType{3, 4.5}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};

    vec.insert(vec.cbegin(), reinterpret_cast<void*>(0x3));
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x3));
  }

  // Inserting at the end
  {
    carry::vector<int> vec{1, 2, 3};

    vec.insert(vec.cend(), 4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[3], 4);
  }

  // Inserting at the beginning
  {
    carry::vector<int> vec{2, 3, 4};

    int i = 1;
    vec.insert(vec.cbegin(), i);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], 1);
  }

  // Inserting into an empty vector
  {
    carry::vector<int> vec;

    int i = 1;
    vec.insert(vec.cbegin(), i);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};

    vec.insert(vec.cbegin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }
}

TEST(ipcpp_vector, insert_const_iterator_rvalue) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    vec.insert(vec.cbegin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }

  {
    carry::vector<double> vec{1.1, 2.2, 3.3};

    vec.insert(vec.cbegin() + 1, 4.4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[1], 4.4);
    EXPECT_EQ(vec[2], 2.2);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};

    vec.insert(vec.cbegin(), "zero");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "zero");
    EXPECT_EQ(vec[1], "one");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};

    vec.insert(vec.cbegin() + 1, {7, 8.5});
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[1], (CustomType{7, 8.5}));
    EXPECT_EQ(vec[2], (CustomType{3, 4.5}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};

    vec.insert(vec.cbegin(), reinterpret_cast<void*>(0x3));
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x3));
  }

  // Inserting at the end
  {
    carry::vector<int> vec{1, 2, 3};

    vec.insert(vec.cend(), 4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[3], 4);
  }

  // Inserting at the beginning
  {
    carry::vector<int> vec{2, 3, 4};

    vec.insert(vec.cbegin(), 1);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], 1);
  }

  // Inserting into an empty vector
  {
    carry::vector<int> vec;

    vec.insert(vec.cbegin(), 1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};

    vec.insert(vec.cbegin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }
}

TEST(ipcpp_vector, insert_count_value) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);
  // Insert multiple values in the middle
  {
    carry::vector<int> vec{1, 2, 6, 7};

    vec.insert(vec.begin() + 2, 3, 5);
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 5, 5, 5, 6, 7}));
  }

  // Insert multiple values at the beginning
  {
    carry::vector<int> vec{4, 5, 6, 7};

    vec.insert(vec.begin(), 2, 1);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{1, 1, 4, 5, 6, 7}));
  }

  // Insert multiple values at the end
  {
    carry::vector<int> vec{10, 20, 30};

    vec.insert(vec.end(), 4, 50);
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 50, 50, 50, 50}));
  }

  // Insert zero values
  {
    carry::vector<int> vec{100, 200, 300};

    vec.insert(vec.begin() + 1, 0, 500);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{100, 200, 300}));
  }

  // Insert into an empty vector
  {
    carry::vector<int> vec;

    vec.insert(vec.begin(), 3, 42);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{42, 42, 42}));
  }

  // Insert into a vector of non-trivial types
  {
    carry::vector<std::string> vec{"alpha", "omega"};

    vec.insert(vec.begin() + 1, 2, "beta");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "beta", "omega"}));
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 6, 7};

    vec.insert(vec.begin() + 2, 3, 5);
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 5, 5, 5, 6, 7}));
  }
}

TEST(ipcpp_vector, insert_input_iterators) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Insert a range of integers in the middle
  {
    carry::vector<int> vec{1, 2, 6, 7};
    std::list<int> range{3, 4, 5};

    vec.insert(vec.begin() + 2, range.begin(), range.end());
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  }

  // Insert a range of integers at the beginning
  {
    carry::vector<int> vec{4, 5, 6};
    std::list<int> range{1, 2, 3};

    vec.insert(vec.begin(), range.begin(), range.end());
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5, 6}));
  }

  // Insert a range of integers at the end
  {
    carry::vector<int> vec{10, 20, 30};
    std::list<int> range{40, 50, 60};

    vec.insert(vec.end(), range.begin(), range.end());
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 40, 50, 60}));
  }

  // Insert an empty range
  {
    carry::vector<int> vec{100, 200, 300};
    std::list<int> range;

    vec.insert(vec.begin() + 1, range.begin(), range.end());
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{100, 200, 300}));
  }

  // Insert a range of strings in the middle
  {
    carry::vector<std::string> vec{"alpha", "delta"};
    std::list<std::string> range{"beta", "gamma"};

    vec.insert(vec.begin() + 1, range.begin(), range.end());
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "gamma", "delta"}));
  }

  // Insert a range from a different container type
  {
    carry::vector<int> vec{10, 20, 30};
    std::list<int> range{40, 50, 60};

    vec.insert(vec.end(), range.begin(), range.end());
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 40, 50, 60}));
  }

  {
    carry::vector<int> vec{1, 2};
    carry::vector<int> range{3, 4, 5};

    vec.insert(vec.begin(), range.begin(), range.end());
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec, (carry::vector<int>{3, 4, 5, 1, 2}));
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 6, 7};
    std::list<int> range{3, 4, 5};

    vec.insert(vec.begin() + 2, range.begin(), range.end());
    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5, 6, 7}));
  }
}

TEST(ipcpp_vector, insert_initializer_list) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Insert initializer list in the middle
  {
    carry::vector<int> vec{1, 2, 6, 7};

    vec.insert(vec.begin() + 2, {3, 4, 5});

    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5, 6, 7}));
  }

  // Insert initializer list at the beginning
  {
    carry::vector<int> vec{4, 5, 6};

    vec.insert(vec.begin(), {1, 2, 3});
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4, 5, 6}));
  }

  // Insert initializer list at the end
  {
    carry::vector<int> vec{10, 20, 30};

    vec.insert(vec.end(), {40, 50, 60});
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 40, 50, 60}));
  }

  // Insert empty initializer list
  {
    carry::vector<int> vec{100, 200, 300};

    vec.insert(vec.begin() + 1, {});
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{100, 200, 300}));
  }

  // Insert initializer list of strings
  {
    carry::vector<std::string> vec{"alpha", "delta"};

    vec.insert(vec.begin() + 1, {"beta", "gamma"});
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "gamma", "delta"}));
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 6, 7};

    vec.insert(vec.begin() + 2, {3, 4, 5});

    EXPECT_EQ(vec.size(), 7);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 4, 5, 6, 7}));
  }
}

TEST(ipcpp_vector, emplace) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    vec.emplace(vec.begin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }

  {
    carry::vector<std::string> vec{"one", "two", "three"};

    vec.emplace(vec.begin(), "zero");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "zero");
    EXPECT_EQ(vec[1], "one");
  }

  {
    carry::vector<CustomType> vec{{1, 2.5}, {3, 4.5}, {5, 6.5}};

    vec.emplace(vec.begin() + 1, 7, 8.5);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[1], (CustomType{7, 8.5}));
    EXPECT_EQ(vec[2], (CustomType{3, 4.5}));
  }

  {
    carry::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};

    vec.emplace(vec.begin(), reinterpret_cast<void*>(0x3));
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], reinterpret_cast<void*>(0x3));
  }

  // Emplace at the end
  {
    carry::vector<int> vec{1, 2, 3};

    vec.emplace(vec.end(), 4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[3], 4);
  }

  // Emplace at the beginning
  {
    carry::vector<int> vec{2, 3, 4};

    vec.emplace(vec.begin(), 1);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], 1);
  }

  // Emplace into an empty vector
  {
    carry::vector<int> vec;

    vec.emplace(vec.begin(), 1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};

    vec.emplace(vec.begin() + 2, 10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 3);
  }
}

TEST(ipcpp_vector, erase_iterator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Erase a single element from the middle
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    auto result = vec.erase(vec.begin() + 2);

    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 4, 5}));
    EXPECT_EQ(*result, 4);
  }

  // Erase the first element
  {
    carry::vector<int> vec{10, 20, 30, 40};

    auto result = vec.erase(vec.begin());
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{20, 30, 40}));
    EXPECT_EQ(*result, 20);
  }

  // Erase the last element
  {
    carry::vector<int> vec{100, 200, 300};

    auto result = vec.erase(vec.end() - 1);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int>{100, 200}));
    EXPECT_EQ(result, vec.end());
  }

  // Erase the only element in the vector
  {
    carry::vector<int> vec{42};

    auto result = vec.erase(vec.begin());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(result, vec.end());
  }

  // Erase from a vector with non-trivial types
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};

    auto result = vec.erase(vec.begin() + 1);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "gamma"}));
    EXPECT_EQ(*result, "gamma");
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};

    auto result = vec.erase(vec.begin() + 2);

    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 4, 5}));
    EXPECT_EQ(*result, 4);
  }
}

TEST(ipcpp_vector, erase_first_last_iterator) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // erase front
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    auto result = vec.erase(vec.begin(), vec.begin() + 1);

    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{2, 3, 4, 5}));
    EXPECT_EQ(*result, 2);
  }

  // erase multiple at front
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};

    auto result = vec.erase(vec.begin(), vec.begin() + 2);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{3, 4, 5}));
    EXPECT_EQ(*result, 3);
  }

  // erase empty range [begin, begin)
  {
    carry::vector<int> vec{10, 20, 30, 40};
    auto it = vec.begin();

    auto result = vec.erase(vec.begin(), vec.begin());
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 40}));
    EXPECT_EQ(*result, 10);
  }

  // erase empty range [begin + 1, begin)
  {
    carry::vector<int> vec{10, 20, 30, 40};
    auto it = vec.begin();

    auto result = vec.erase(vec.begin() + 1, vec.begin());
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{10, 20, 30, 40}));
    EXPECT_EQ(*result, 10);
  }

  // erase last elements
  {
    carry::vector<int> vec{100, 200, 300};

    auto result = vec.erase(vec.end() - 2, vec.end());
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec, (carry::vector<int>{100}));
    EXPECT_EQ(result, vec.end());
  }

  // erase back
  {
    carry::vector<int> vec{100, 200, 300};

    auto result = vec.erase(vec.end() - 1, vec.end());
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int>{100, 200}));
    EXPECT_EQ(result, vec.end());
  }

  // erase the only element in the vector
  {
    carry::vector<int> vec{42};
    auto it = vec.begin();

    auto result = vec.erase(vec.begin(), vec.end());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(result, vec.end());
  }

  // erase all elements
  {
    carry::vector<int> vec{1, 2, 3, 4};
    auto it = vec.begin();

    auto result = vec.erase(vec.begin(), vec.end());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(result, vec.end());
  }

  // Erase from a vector with non-trivial types
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};

    auto result = vec.erase(vec.begin() + 1, vec.end());
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha"}));
    EXPECT_EQ(result, vec.end());
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4, 5};

    auto result = vec.erase(vec.begin(), vec.begin() + 1);

    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{2, 3, 4, 5}));
    EXPECT_EQ(*result, 2);
  }
}

TEST(ipcpp_vector, push_back_rvalue) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Push back single element
  {
    carry::vector<int> vec;
    vec.push_back(10);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1);
    EXPECT_EQ(vec[0], 10);
  }

  // Push back multiple elements
  {
    carry::vector<int> vec{1, 2};
    vec.push_back(3);
    vec.push_back(4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4}));
  }

  // Push back into a vector of non-trivial types
  {
    carry::vector<std::string> vec{"hello", "world"};
    vec.push_back("!");
    vec.push_back(":)");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"hello", "world", "!", ":)"}));
  }

  // Push back into an initially reserved vector
  {
    carry::vector<int> vec;
    vec.reserve(5);
    vec.push_back(42);
    vec.push_back(43);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int>{42, 43}));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec.push_back(10);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1);
    EXPECT_EQ(vec[0], 10);
  }
}

TEST(ipcpp_vector, push_back_reference) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Push back single element
  {
    carry::vector<int> vec;
    int i = 10;
    vec.push_back(i);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1);
    EXPECT_EQ(vec[0], 10);
  }

  // Push back into a vector of non-trivial types
  {
    carry::vector<std::string> vec{"hello", "world"};
    std::string a("!");
    vec.push_back(a);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<std::string>{"hello", "world", "!"}));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    int i = 10;
    vec.push_back(i);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1);
    EXPECT_EQ(vec[0], 10);
  }
}

TEST(ipcpp_vector, emplace_back) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Emplace back single element
  {
    carry::vector<int> vec;
    vec.emplace_back(10);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 10);
  }

  // Emplace back multiple elements
  {
    carry::vector<int> vec{1, 2};
    vec.emplace_back(3);
    vec.emplace_back(4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 4}));
  }

  // Emplace back into a vector of non-trivial types
  {
    carry::vector<CustomType> vec;
    vec.emplace_back(1, 0.9);
    vec.emplace_back(2, 1.9);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0].a, 1);
    EXPECT_EQ(vec[0].b, 0.9);
    EXPECT_EQ(vec[1].a, 2);
    EXPECT_EQ(vec[1].b, 1.9);
  }

  // Emplace back into a vector of non-trivial types
  {
    carry::vector<std::string> vec{"hello", "world"};
    vec.emplace_back("!");
    vec.emplace_back(":)", 5);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"hello", "world", "!", std::string(":)", 5)}));
    EXPECT_EQ(vec.back().size(), 5);
  }

  // Emplace back into an initially reserved vector
  {
    carry::vector<int> vec;
    vec.reserve(5);
    vec.emplace_back(42);
    vec.emplace_back(43);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int>{42, 43}));
  }

  {
    carry::vector<int, std::allocator<int>> vec;
    vec.emplace_back(10);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 10);
  }
}

TEST(ipcpp_vector, pop_back) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Pop back from a vector with multiple elements
  {
    carry::vector<int> vec{1, 2, 3, 4};
    vec.pop_back();
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3}));
    vec.pop_back();
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2}));
  }

  // Pop back from a vector with one element
  {
    carry::vector<int> vec{100};
    vec.pop_back();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  // Pop back from a vector of non-trivial types
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};
    vec.pop_back();
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta"}));
  }

  // Pop back until empty
  {
    carry::vector<int> vec{10, 20, 30};
    vec.pop_back();
    vec.pop_back();
    vec.pop_back();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3, 4};
    vec.pop_back();
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3}));
    vec.pop_back();
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2}));
  }
}

TEST(ipcpp_vector, resize) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Resize to a larger size with default value
  {
    carry::vector<int> vec{1, 2, 3};
    vec.resize(6);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 0, 0, 0}));
  }

  // Resize to a smaller size
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};
    vec.resize(3);  // Resize to a smaller size
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3}));
  }

  // Resize to the same size
  {
    carry::vector<int> vec{1, 2, 3};
    vec.resize(3);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3}));
  }

  // Resize a vector of non-trivial types to a larger size
  {
    carry::vector<std::string> vec{"alpha", "beta"};
    vec.resize(4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", std::string(), std::string()}));
  }

  // Resize a vector of non-trivial types to a smaller size
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};
    vec.resize(1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha"}));
  }

  // Resize a vector that has already reserved the requested size
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};
    vec.reserve(6);
    vec.resize(6);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "gamma", "", "", ""}));
  }

  // Resize a vector to a size larger than max_size()
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};
    EXPECT_THROW(vec.resize(10000), std::length_error);
  }

  // Resize a vector to the size it already has
  {
    carry::vector<int> vec{1, 2, 3};
    vec.resize(3);
    EXPECT_EQ(vec.size(), 3);
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3};
    vec.resize(6);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 0, 0, 0}));
  }
}

TEST(ipcpp_vector, resize_with_value) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Resize to a larger size with a specified value
  {
    carry::vector<int> vec{1, 2, 3};
    vec.resize(6, 42);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3, 42, 42, 42}));
  }

  // Resize to a smaller size (value should not matter)
  {
    carry::vector<int> vec{1, 2, 3, 4, 5};
    vec.resize(3, 42);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{1, 2, 3}));
  }

  // Resize with a custom value for a vector of non-trivial types
  {
    carry::vector<std::string> vec{"alpha", "beta"};
    vec.resize(4, "default");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "default", "default"}));
  }

  // Resize an empty vector with a custom value
  {
    carry::vector<int> vec;
    vec.resize(3, 7);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec, (carry::vector<int>{7, 7, 7}));
  }

  // Resize to a larger size with a default-initialized custom type
  {
    carry::vector<CustomType> vec{{1, 0.1}, {2, 0.2}};
    vec.resize(5, {42, 1.0});
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec, (carry::vector<CustomType>{{1, 0.1}, {2, 0.2}, {42, 1.0}, {42, 1.0}, {42, 1.0}}));
  }

  // Resize a vector that has already reserved the requested size
  {
    carry::vector<std::string> vec{"alpha", "beta"};
    vec.reserve(4);
    vec.resize(4, "default");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec, (carry::vector<std::string>{"alpha", "beta", "default", "default"}));
  }

  // Resize a vector to a size larger than max_size()
  {
    carry::vector<std::string> vec{"alpha", "beta", "gamma"};
    EXPECT_THROW(vec.resize(10000, "hi"), std::length_error);
  }

  // Resize a vector to the size it already has
  {
    carry::vector<int> vec{1, 2, 3};
    vec.resize(3, 5);
    EXPECT_EQ(vec.size(), 3);
  }

  {
    carry::vector<int, std::allocator<int>> vec{1, 2, 3};
    vec.resize(6, 42);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec, (carry::vector<int, std::allocator<int>>{1, 2, 3, 42, 42, 42}));
  }
}

TEST(ipcpp_vector, swap) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Swap two vectors of equal size
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{4, 5, 6};
    vec1.swap(vec2);
    EXPECT_EQ(vec1, (carry::vector<int>{4, 5, 6}));
    EXPECT_EQ(vec2, (carry::vector<int>{1, 2, 3}));
  }

  // Swap two vectors of different sizes
  {
    carry::vector<int> vec1{10, 20};
    carry::vector<int> vec2{30, 40, 50, 60};
    vec1.swap(vec2);
    EXPECT_EQ(vec1, (carry::vector<int>{30, 40, 50, 60}));
    EXPECT_EQ(vec2, (carry::vector<int>{10, 20}));
  }

  // Swap with an empty vector
  {
    carry::vector<int> vec1{100, 200, 300};
    carry::vector<int> vec2;
    vec1.swap(vec2);
    EXPECT_TRUE(vec1.empty());
    EXPECT_EQ(vec2, (carry::vector<int>{100, 200, 300}));
  }

  // Swap two empty vectors
  {
    carry::vector<int> vec1;
    carry::vector<int> vec2;
    vec1.swap(vec2);
    EXPECT_TRUE(vec1.empty());
    EXPECT_TRUE(vec2.empty());
  }

  // Swap vectors with non-trivial types
  {
    carry::vector<std::string> vec1{"alpha", "beta"};
    carry::vector<std::string> vec2{"gamma", "delta", "epsilon"};
    vec1.swap(vec2);
    EXPECT_EQ(vec1, (carry::vector<std::string>{"gamma", "delta", "epsilon"}));
    EXPECT_EQ(vec2, (carry::vector<std::string>{"alpha", "beta"}));
  }

  {
    carry::vector<int, std::allocator<int>> vec1{1, 2, 3};
    carry::vector<int, std::allocator<int>> vec2{4, 5, 6};
    vec1.swap(vec2);
    EXPECT_EQ(vec1, (carry::vector<int, std::allocator<int>>{4, 5, 6}));
    EXPECT_EQ(vec2, (carry::vector<int, std::allocator<int>>{1, 2, 3}));
  }
}

TEST(ipcpp_vector, operator_equal) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Compare two equal vectors
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{1, 2, 3};
    EXPECT_TRUE(vec1 == vec2);
  }

  // Compare two vectors with different sizes
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{1, 2};
    EXPECT_FALSE(vec1 == vec2);
  }

  // Compare two vectors with different elements
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{1, 2, 4};
    EXPECT_FALSE(vec1 == vec2);
  }

  // Compare an empty vector with another empty vector
  {
    carry::vector<int> vec1;
    carry::vector<int> vec2;
    EXPECT_TRUE(vec1 == vec2);
  }

  // Compare a vector with itself
  {
    carry::vector<int> vec{1, 2, 3};
    EXPECT_TRUE(vec == vec);
  }

  // Compare vectors with non-trivial types
  {
    carry::vector<std::string> vec1{"alpha", "beta", "gamma"};
    carry::vector<std::string> vec2{"alpha", "beta", "gamma"};
    EXPECT_TRUE(vec1 == vec2);

    carry::vector<std::string> vec3{"alpha", "beta", "delta"};
    EXPECT_FALSE(vec1 == vec3);
  }

  {
    carry::vector<int, std::allocator<int>> vec1{1, 2, 3};
    carry::vector<int, std::allocator<int>> vec2{1, 2, 3};
    EXPECT_TRUE(vec1 == vec2);
  }
}

TEST(ipcpp_vector, operator_spaceship) {
  carry::pool_allocator<int>::initialize_factory(reinterpret_cast<std::uintptr_t>(alloc_mem), allocator_mem_size);

  // Compare vectors that are equal
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{1, 2, 3};
    EXPECT_TRUE((vec1 <=> vec2) == 0);
  }

  // Compare vectors where one is less
  {
    carry::vector<int> vec1{1, 2, 3};
    carry::vector<int> vec2{1, 2, 4};
    EXPECT_TRUE((vec1 <=> vec2) < 0);
    EXPECT_TRUE((vec2 <=> vec1) > 0);
  }

  // Compare vectors of different sizes
  {
    carry::vector<int> vec1{1, 2};
    carry::vector<int> vec2{1, 2, 3};
    EXPECT_TRUE((vec1 <=> vec2) < 0);
    EXPECT_TRUE((vec2 <=> vec1) > 0);
  }

  // Compare an empty vector with a non-empty vector
  {
    carry::vector<int> vec1;
    carry::vector<int> vec2{1};
    EXPECT_TRUE((vec1 <=> vec2) < 0);
    EXPECT_TRUE((vec2 <=> vec1) > 0);
  }

  // Compare vectors with non-trivial types
  {
    carry::vector<std::string> vec1{"alpha", "beta", "gamma"};
    carry::vector<std::string> vec2{"alpha", "beta", "gamma"};
    carry::vector<std::string> vec3{"alpha", "beta", "delta"};
    EXPECT_TRUE((vec1 <=> vec2) == 0);
    EXPECT_TRUE((vec1 <=> vec3) > 0);
    EXPECT_TRUE((vec3 <=> vec1) < 0);
  }

  // Self-comparison
  {
    carry::vector<int> vec{1, 2, 3};
    EXPECT_TRUE((vec <=> vec) == 0);
  }

  {
    carry::vector<int, std::allocator<int>> vec1{1, 2, 3};
    carry::vector<int, std::allocator<int>> vec2{1, 2, 3};
    EXPECT_TRUE((vec1 <=> vec2) == 0);
  }
}
