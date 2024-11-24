/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <gtest/gtest.h>

#include <ipcpp/datatypes/vector.h>
#include <ipcpp/shm/dynamic_allocator.h>

constexpr static std::size_t allocator_mem_size = 4096;
static uint8_t alloc_mem[allocator_mem_size];

TEST(ipcpp_vector, constructors_tp_trivial_copyable) {
  ipcpp::shm::DynamicAllocator<int> allocator(alloc_mem, allocator_mem_size);
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(allocator);
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 0);
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(5, allocator);
    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (auto v : vec) {
      ASSERT_EQ(v, 0);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(5, 3, allocator);
    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (auto v : vec) {
      ASSERT_EQ(v, 3);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(5, 3, allocator);
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec2(vec);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (auto v : vec2) {
      ASSERT_EQ(v, 3);
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i], vec2[i]);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(5, 3, allocator);
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec2(std::move(vec));
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec2.capacity(), 5);
    for (auto v : vec2) {
      ASSERT_EQ(v, 3);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(std::initializer_list<int>{0, 1, 2, 3, 4}, allocator);
    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (int i = 0; i < vec.size(); ++i) {
      ASSERT_EQ(i, vec[i]);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec(5, 3, allocator);
    ipcpp::vector<int, ipcpp::shm::DynamicAllocator<int>> vec2(vec.begin(), vec.end(), allocator);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (auto v : vec2) {
      ASSERT_EQ(v, 3);
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i], vec2[i]);
    }
  }
}

TEST(ipcpp_vector, constructors_tp_copy_constructable) {
  ipcpp::shm::DynamicAllocator<std::string> allocator(alloc_mem, allocator_mem_size);
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(allocator);
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 0);
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(5, allocator);

    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (const auto& v : vec) {
      ASSERT_EQ(v, "");
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(5, "hello", allocator);

    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (const auto& v : vec) {
      ASSERT_EQ(v, "hello");
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(5, "hello", allocator);
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec2(vec);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v, "hello");
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i], vec2[i]);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(5, "hello", allocator);
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec2(std::move(vec));
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v, "hello");
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(std::initializer_list<std::string>{"hi", "hello"}, allocator);
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec.capacity(), 2);
    ASSERT_EQ(vec[0], "hi");
    ASSERT_EQ(vec[1], "hello");
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec(5, "hello", allocator);
    ipcpp::vector<std::string, ipcpp::shm::DynamicAllocator<std::string>> vec2(vec.begin(), vec.end(), allocator);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v, "hello");
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i], vec2[i]);
    }
  }
}

TEST(ipcpp_vector, constructors_tp_ipcpp_vector) {
  ipcpp::shm::DynamicAllocator<ipcpp::vector<int>> allocator(alloc_mem, allocator_mem_size);
  ipcpp::shm::DynamicAllocator<int> allocator2(alloc_mem);
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(allocator);
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 0);
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int> init(3, allocator2);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(5, init, allocator);

    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (const auto& v : vec) {
      ASSERT_EQ(v.size(), 3);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int> init(5, allocator2);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(5, init, allocator);

    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec.capacity(), 5);
    for (const auto& v : vec) {
      ASSERT_EQ(v.size(), 5);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int> init(4, allocator2);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(5, init, allocator);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec2(vec);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v.size(), 4);
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i].size(), vec2[i].size());
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int> init(6, allocator2);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(5, init, allocator);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec2(std::move(vec));
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v.size(), 6);
    }
  }
  ASSERT_EQ(allocator.allocated_size(), 0);
  {
    ipcpp::vector<int> init(6, allocator2);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec(5, init, allocator);
    ipcpp::vector<ipcpp::vector<int>, ipcpp::shm::DynamicAllocator<ipcpp::vector<int>>> vec2(vec.begin(), vec.end(), allocator);
    ASSERT_EQ(vec2.size(), 5);
    ASSERT_EQ(vec.size(), vec2.size());
    ASSERT_EQ(vec2.capacity(), 5);
    for (const auto& v : vec2) {
      ASSERT_EQ(v.size(), 6);
    }
    for (std::size_t i = 0; i < vec.size(); ++i) {
      ASSERT_NE(&vec[i], &vec2[i]);
      ASSERT_EQ(vec[i].size(), vec2[i].size());
    }
  }
}