/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <iostream>
#include <array>

using namespace std::chrono_literals;

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

constexpr std::size_t num_elements = 100'000;

std::array<int64_t, num_elements * 2> generate_latencies;

alignas(64) std::atomic_int64_t ts{0};

void starter() {
  for (std::size_t i = 0; i < num_elements; ++i) {
    std::int64_t l_ts = timestamp();
    ts.store(l_ts, std::memory_order_release);
    while (true) {
      if (const auto val = ts.load(std::memory_order_acquire); val != l_ts) {
        l_ts = timestamp();
        generate_latencies[i + num_elements] = l_ts - val;
        break;
      }
    }
  }
}

void responder() {
  std::int64_t last_l_ts = ts.load(std::memory_order_acquire);
  for (std::size_t i = 0; i < num_elements; ++i) {
    while (true) {
      if (const auto val = ts.load(std::memory_order_acquire); val != last_l_ts) {
        const auto l_ts = timestamp();
        generate_latencies[i] = l_ts - val;
        break;
      }
    }
    last_l_ts = timestamp();
    ts.store(last_l_ts, std::memory_order_release);
  }
}

int main() {
  std::thread observer_thread(responder);
  std::this_thread::sleep_for(100ms);
  std::thread notifier_thread(starter);

  observer_thread.join();
  notifier_thread.join();

  std::cout << "latency: " << std::accumulate(generate_latencies.begin(), generate_latencies.end(), 0.0f) /
                                    static_cast<double>(generate_latencies.size()) << "ns" << std::endl;
}