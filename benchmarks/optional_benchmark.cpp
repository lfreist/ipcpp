/**
 * Copyright 2025, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <memory>
#include <iostream>
#include <chrono>
#include <thread>

#include <ipcpp/utils/utils.h>
#include <ipcpp/stl/optional.h>

using namespace std::chrono_literals;

constexpr auto iterations = 1'000;

struct Data {
  // with this constructor, d is uninitialized and std::optional<Data>::emplace() is fast
  Data() {};
  // with a trivial default constructor each std::uint64_t gets explicitly constructed when calling
  //  std::optional<Data>::emplace() which makes it quite slow!
  // Data() = default;
  std::uint64_t d[8192 << 2];
};

void benchmark_std_optional() {
  std::optional<Data> opt_data;
  auto start = ipcpp::utils::timestamp();
  for (std::size_t i = 0; i < iterations; ++i) {
    opt_data.emplace();
  }
  auto end = ipcpp::utils::timestamp();

  std::cout << "std::optional::emplace: " << (end - start) << "ns (" << (end-start)/iterations << "ns per emplace)" << std::endl;
}

void benchmark_ipcpp_optional() {
  ipcpp::optional<Data> opt_data;
  auto start = ipcpp::utils::timestamp();
  for (std::size_t i = 0; i < iterations; ++i) {
    opt_data.emplace();

  }
  auto end = ipcpp::utils::timestamp();

  std::cout << "ipcpp::optional::emplace: " << (end - start) << "ns (" << (end-start)/iterations << "ns per emplace)" << std::endl;
}

int main() {
  benchmark_std_optional();
  std::this_thread::sleep_for(1s);
  benchmark_ipcpp_optional();
  std::this_thread::sleep_for(1s);
  benchmark_std_optional();
  std::this_thread::sleep_for(1s);
  benchmark_ipcpp_optional();
}