/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <numeric>
#include <print>
#include <thread>

using namespace std::chrono_literals;

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::vector<int64_t> generate_latencies;

void signal_handler(int signal)
{
  std::println("latency: {}ns", std::accumulate(generate_latencies.begin(), generate_latencies.end(), 0.0f) / static_cast<double>(generate_latencies.size()));
  std::exit(0);
}

void notifier(std::atomic_size_t& counter, int64_t& sender_timestamp) {
  while (true) {
    for (volatile int i = 0; i < 10000; i++) {}
    sender_timestamp = timestamp();
    counter.fetch_add(1, std::memory_order_release);
  }
}

void observer(const std::atomic_size_t& counter, std::size_t& initial, const int64_t& sender_timestamp) {
  while (true) {
    if (auto val = counter.load(std::memory_order_acquire); val != initial) {
      auto ts = timestamp();
      // std::println("# {}, latency: {}ns", val, ts - sender_timestamp);
      generate_latencies.push_back(ts - sender_timestamp);
      initial = val;
    }
    // std::this_thread::yield();
  }
}

int main() {
  std::signal(SIGTERM, signal_handler);
  generate_latencies.reserve(10000000);
  alignas(64) std::atomic_size_t counter{0};
  alignas(64) std::size_t initial{0};
  alignas(64) std::int64_t sender_timestamp{0};
  std::thread notifier_thread(notifier, std::ref(counter), std::ref(sender_timestamp));
  std::thread observer_thread(observer, std::ref(counter), std::ref(initial), std::ref(sender_timestamp));

  notifier_thread.join();
  observer_thread.join();
}