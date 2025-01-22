/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <iostream>
#include <array>
#include <barrier>

using namespace std::chrono_literals;

std::barrier sync_point(3);
std::barrier benchmark_start_barrier(3);

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

constexpr std::size_t num_iterations = 1'000'000;

alignas(64) std::atomic_int64_t a2b{0};
alignas(64) std::atomic_int64_t b2a{0};

void starter() {
  std::int64_t last_b2a = 0;
  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();
  for (std::size_t i = 0; i < num_iterations; ++i) {
    a2b.fetch_add(1, std::memory_order_relaxed);
    while (true) {
      if (const auto val = b2a.load(std::memory_order_relaxed); val != last_b2a) {
        last_b2a = val;
        break;
      }
    }
  }
}

void responder() {
  std::int64_t last_a2b = 0;
  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();
  for (std::size_t i = 0; i < num_iterations; ++i) {
    while (true) {
      if (const auto val = a2b.load(std::memory_order_acquire); val != last_a2b) {
        last_a2b = val;
        break;
      }
    }
    b2a.fetch_add(1, std::memory_order_relaxed);
  }
}

int main() {
  std::thread observer_thread(responder);
  std::this_thread::sleep_for(100ms);
  std::thread notifier_thread(starter);

  sync_point.arrive_and_wait();
  auto start = timestamp();
  benchmark_start_barrier.arrive_and_wait();
  
  observer_thread.join();
  notifier_thread.join();

  auto stop = timestamp();
  
  std::cout << "Iterations: " << num_iterations << "\n"
            << "Time:       " << stop - start << "ns\n"
            << "Latency:    " << static_cast<double>(stop - start) / static_cast<double>(2 * num_iterations) << "ns"
            << std::endl;
  return 0;
}