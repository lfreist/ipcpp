/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <gtest/gtest.h>
#include <ipcpp/container/vector.h>
#include <ipcpp/shm/dynamic_allocator.h>

#include <list>

constexpr static std::size_t allocator_mem_size = 4096;
static uint8_t alloc_mem[allocator_mem_size];

TEST(ipcpp_vector, default_constructor) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    ipcpp::vector<double> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    ipcpp::vector<std::string> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    struct CustomType {
      int a;
      double b;
    };

    ipcpp::vector<CustomType> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    struct EmptyStruct {};

    ipcpp::vector<EmptyStruct> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }

  {
    ipcpp::vector<std::unique_ptr<int>> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
  }
}

TEST(ipcpp_vector, constructor_size_value) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> vec(5, 42);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](int v) { return v == 42; }));
  }

  {
    ipcpp::vector<double> vec(3, 3.14);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](double v) { return v == 3.14; }));
  }

  {
    ipcpp::vector<std::string> vec(4, "test");
    EXPECT_EQ(vec.size(), 4);
    EXPECT_GE(vec.capacity(), 4);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](const std::string& v) { return v == "test"; }));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    CustomType value{1, 2.5};
    ipcpp::vector<CustomType> vec(2, value);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [&](const CustomType& v) { return v == value; }));
  }

  {
    struct EmptyStruct {};
    ipcpp::vector<EmptyStruct> vec(6, EmptyStruct{});
    EXPECT_EQ(vec.size(), 6);
    EXPECT_GE(vec.capacity(), 6);
  }
}

TEST(ipcpp_vector, copy_constructor) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> original{1, 2, 3, 4, 5};
    ipcpp::vector<int> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.data(), copy.data() + copy.size(), original.data()));
  }

  {
    ipcpp::vector<double> original{3.14, 2.71, 1.61};
    ipcpp::vector<double> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.data(), copy.data() + copy.size(), original.data()));
  }

  {
    ipcpp::vector<std::string> original{"one", "two", "three"};
    ipcpp::vector<std::string> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.data(), copy.data() + copy.size(), original.data()));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> original{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.data(), copy.data() + copy.size(), original.begin()));
  }

  {
    ipcpp::vector<void*> original{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    ipcpp::vector<void*> copy(original);
    EXPECT_EQ(copy.size(), original.size());
    EXPECT_EQ(copy.capacity(), original.capacity());
    EXPECT_TRUE(std::equal(copy.data(), copy.data() + copy.size(), original.begin()));
  }

  {
    ipcpp::vector<std::unique_ptr<int>> original;
    original.push_back(std::make_unique<int>(10));
    original.push_back(std::make_unique<int>(20));

    // Cannot copy std::unique_ptr, so this tests that copy is unavailable
    static_assert(!std::is_copy_constructible_v<ipcpp::vector<std::unique_ptr<int>>>);
  }
}

TEST(ipcpp_vector, move_constructor) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> original{1, 2, 3, 4, 5};
    ipcpp::vector<int> moved(std::move(original));
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(moved.size(), 5);
    EXPECT_GE(moved.capacity(), 5);
    EXPECT_EQ(moved, (ipcpp::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    ipcpp::vector<double> original{3.14, 2.71, 1.61};
    ipcpp::vector<double> moved(std::move(original));
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(moved.size(), 3);
    EXPECT_GE(moved.capacity(), 3);
    EXPECT_EQ(moved, ipcpp::vector<double>({3.14, 2.71, 1.61}));
  }

  {
    ipcpp::vector<std::string> original{"one", "two", "three"};
    ipcpp::vector<std::string> moved(std::move(original));
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(moved.size(), 3);
    EXPECT_GE(moved.capacity(), 3);
    EXPECT_EQ(moved, (ipcpp::vector<std::string>{"one", "two", "three"}));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
      bool operator<(const CustomType& other) const { return a < other.a; }
    };

    ipcpp::vector<CustomType> original{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> moved(std::move(original));
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(moved.size(), 2);
    EXPECT_GE(moved.capacity(), 2);
    EXPECT_EQ(moved, (ipcpp::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
    std::cout << "destructing..." << std::endl;
  }

  {
    ipcpp::vector<std::unique_ptr<int>> original;
    original.push_back(std::make_unique<int>(10));
    original.push_back(std::make_unique<int>(20));
    ipcpp::vector<std::unique_ptr<int>> moved(std::move(original));
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(moved.size(), 2);
    EXPECT_EQ(*moved[0], 10);
    EXPECT_EQ(*moved[1], 20);
  }
}

TEST(ipcpp_vector, constructor_initializer_list) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> vec{1, 2, 3, 4, 5};
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (ipcpp::vector<int>{1, 2, 3, 4, 5}));
  }
  
  {
    ipcpp::vector<double> vec{3.14, 2.71, 1.61};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (ipcpp::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    ipcpp::vector<std::string> vec{"one", "two", "three"};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (ipcpp::vector<std::string>{"one", "two", "three"}));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> vec{{1, 2.5}, {3, 4.5}};
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (ipcpp::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    ipcpp::vector<void*> vec{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (ipcpp::vector<void*>{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)}));
  }
}

TEST(ipcpp_vector, constructor_iterator) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> source{1, 2, 3, 4, 5};
    ipcpp::vector<int> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }
  
  {
    ipcpp::vector<double> source{3.14, 2.71, 1.61};
    ipcpp::vector<double> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    ipcpp::vector<std::string> source{"one", "two", "three"};
    ipcpp::vector<std::string> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    ipcpp::vector<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    ipcpp::vector<void*> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<int> source{10, 20, 30, 40};
    ipcpp::vector<int> vec(source.begin(), source.end());
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_TRUE(std::equal(vec.data(), vec.data() + vec.size(), source.begin()));
  }
}

struct Tracker {
  static inline int active_count = 0;
  Tracker() { ++active_count; }
  ~Tracker() { --active_count; }
};

TEST(ipcpp_vector, destructor) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<Tracker> vec(5);
    EXPECT_EQ(Tracker::active_count, 5);
  }

  EXPECT_EQ(Tracker::active_count, 0);

  {
    ipcpp::vector<std::unique_ptr<int>> vec;
    vec.push_back(std::make_unique<int>(10));
    vec.push_back(std::make_unique<int>(20));
    EXPECT_EQ(vec.size(), 2);
  }
}

TEST(ipcpp_vector, move_assign_operator) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> source{1, 2, 3, 4, 5};
    ipcpp::vector<int> target;
    target = std::move(source);
    
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(target.size(), 5);
    EXPECT_GE(target.capacity(), 5);
    EXPECT_EQ(target, (ipcpp::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    ipcpp::vector<double> source{3.14, 2.71, 1.61};
    ipcpp::vector<double> target;
    target = std::move(source);

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(target.size(), 3);
    EXPECT_GE(target.capacity(), 3);
    EXPECT_EQ(target, (ipcpp::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    ipcpp::vector<std::string> source{"one", "two", "three"};
    ipcpp::vector<std::string> target;
    target = std::move(source);

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(target.size(), 3);
    EXPECT_GE(target.capacity(), 3);
    EXPECT_EQ(target, (ipcpp::vector<std::string>{"one", "two", "three"}));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> target;
    target = std::move(source);

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(target.size(), 2);
    EXPECT_GE(target.capacity(), 2);
    EXPECT_EQ(target, (ipcpp::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    ipcpp::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(10));
    source.push_back(std::make_unique<int>(20));

    ipcpp::vector<std::unique_ptr<int>> target;
    target = std::move(source);

    EXPECT_TRUE(source.empty());
    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(*target[0], 10);
    EXPECT_EQ(*target[1], 20);
  }
}

TEST(ipcpp_vector, copy_assign_operator) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> source{1, 2, 3, 4, 5};
    ipcpp::vector<int> target;
    target = source;
    
    EXPECT_EQ(source.size(), 5);
    EXPECT_EQ(target.size(), 5);
    EXPECT_EQ(source, target);
  }

  {
    ipcpp::vector<double> source{3.14, 2.71, 1.61};
    ipcpp::vector<double> target;
    target = source;

    EXPECT_EQ(source.size(), 3);
    EXPECT_EQ(target.size(), 3);
    EXPECT_EQ(source, target);
  }

  {
    ipcpp::vector<std::string> source{"one", "two", "three"};
    ipcpp::vector<std::string> target;
    target = source;

    EXPECT_EQ(source.size(), 3);
    EXPECT_EQ(target.size(), 3);
    EXPECT_EQ(source, target);
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };
    
    ipcpp::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> target;
    target = source;

    EXPECT_EQ(source.size(), 2);
    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(source, target);
  }

  {
    ipcpp::vector<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    ipcpp::vector<void*> target;
    target = source;

    EXPECT_EQ(source.size(), 2);
    EXPECT_EQ(target.size(), 2);
    EXPECT_EQ(source, target);
  }
}

TEST(ipcpp_vector, assign_size_value) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> vec;
    vec.assign(5, 42);
    
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](int x) { return x == 42; }));
  }

  {
    ipcpp::vector<double> vec;
    vec.assign(3, 3.14);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](double x) { return x == 3.14; }));
  }

  {
    ipcpp::vector<std::string> vec;
    vec.assign(4, "test");

    EXPECT_EQ(vec.size(), 4);
    EXPECT_GE(vec.capacity(), 4);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](const std::string& s) { return s == "test"; }));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };
    
    ipcpp::vector<CustomType> vec;
    vec.assign(2, {42, 3.14});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_TRUE(std::all_of(vec.data(), vec.data() + vec.size(), [](const CustomType& obj) { return obj.a == 42 && obj.b == 3.14; }));
  }
}

TEST(ipcpp_vector, assign_iterator) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> source{1, 2, 3, 4, 5};
    ipcpp::vector<int> vec;
    vec.assign(source.begin(), source.end());
    
    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<double> source{3.14, 2.71, 1.61};
    ipcpp::vector<double> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.data(), vec.data() + vec.size(), source.begin()));
  }

  {
    std::list<std::string> source{"one", "two", "three"};
    ipcpp::vector<std::string> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.data(), vec.data() + vec.size(), source.begin()));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> source{{1, 2.5}, {3, 4.5}};
    ipcpp::vector<CustomType> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_EQ(vec, source);
  }

  {
    std::list<void*> source{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)};
    ipcpp::vector<void*> vec;
    vec.assign(source.begin(), source.end());

    EXPECT_EQ(vec.size(), source.size());
    EXPECT_GE(vec.capacity(), source.size());
    EXPECT_TRUE(std::equal(vec.data(), vec.data() + vec.size(), source.begin()));
  }
}

TEST(ipcpp_vector, assign_initializer_list) {
  ipcpp::shm::DynamicAllocator<int>::initialize_factory(alloc_mem, allocator_mem_size);
  {
    ipcpp::vector<int> vec;
    vec.assign({1, 2, 3, 4, 5});
    
    EXPECT_EQ(vec.size(), 5);
    EXPECT_GE(vec.capacity(), 5);
    EXPECT_EQ(vec, (ipcpp::vector<int>{1, 2, 3, 4, 5}));
  }

  {
    ipcpp::vector<double> vec;
    vec.assign({3.14, 2.71, 1.61});

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (ipcpp::vector<double>{3.14, 2.71, 1.61}));
  }

  {
    ipcpp::vector<std::string> vec;
    vec.assign({"one", "two", "three"});

    EXPECT_EQ(vec.size(), 3);
    EXPECT_GE(vec.capacity(), 3);
    EXPECT_EQ(vec, (ipcpp::vector<std::string>{"one", "two", "three"}));
  }

  {
    struct CustomType {
      int a;
      double b;
      bool operator==(const CustomType& other) const { return a == other.a && b == other.b; }
    };

    ipcpp::vector<CustomType> vec;
    vec.assign({{1, 2.5}, {3, 4.5}});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (ipcpp::vector<CustomType>{{1, 2.5}, {3, 4.5}}));
  }

  {
    ipcpp::vector<void*> vec;
    vec.assign({reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_GE(vec.capacity(), 2);
    EXPECT_EQ(vec, (ipcpp::vector<void*>{reinterpret_cast<void*>(0x1), reinterpret_cast<void*>(0x2)}));
  }
}
