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

constexpr const int num_iterations = 1000000;

inline int64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::vector<int64_t> generate_latencies(num_iterations);

alignas(64) std::atomic_size_t counter{0};
alignas(64) std::size_t initial{0};
alignas(64) std::int64_t sender_timestamp{0};
alignas(64) std::size_t sink{0};

void signal_handler(int signal)
{
  std::println("latency: {}ns (counter: {}, initial: {})", std::accumulate(generate_latencies.begin(), generate_latencies.end(), 0.0f) / static_cast<double>(generate_latencies.size()), counter.load(), initial);
  std::println("sink: {}", sink);
  std::exit(0);
}

void notifier() {
  for (std::size_t j = 0; j < num_iterations; ++j) {
    for (std::size_t i = 0; i < 10000; ++i) {
      sink += i;
      sink -= j % 100;
    }
    sender_timestamp = timestamp();
    counter.fetch_add(1, std::memory_order_release);
  }
}

void observer() {
  for (auto& v : generate_latencies) {
    while (true) {
      if (const auto val = counter.load(std::memory_order_acquire); val != initial) {
        const auto ts = timestamp();
        v = ts - sender_timestamp;
        initial = val;
        break;
      }
    }
  }
}

int main() {
  std::signal(SIGTERM, signal_handler);
  std::thread observer_thread(observer);
  std::this_thread::sleep_for(100ms);
  std::thread notifier_thread(notifier);

  notifier_thread.join();
  observer_thread.join();

  std::println("latency: {}ns (counter: {}, initial: {})", std::accumulate(generate_latencies.begin(), generate_latencies.end(), 0.0f) / static_cast<double>(generate_latencies.size()), counter.load(), initial);
}