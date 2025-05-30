/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */
#include <ipcpp/publish_subscribe/rt_shm_layout.h>
#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/publish_subscribe/real_time_subscriber.h>
#include <ipcpp/utils/utils.h>

#include <barrier>
#include <chrono>
#include <numeric>
#include <thread>

using namespace std::chrono_literals;

std::barrier sync_point(3);
std::barrier benchmark_start_barrier(3);
std::barrier benchmark_stop_barrier(3);

constexpr auto num_iterations = 1'000'000;

struct Message {
  Message() {}
  Message(std::uint64_t val) : data(val) {}
  std::uint64_t data;
};

void a2b() {
  auto publisher_a2b = ipcpp::ps::RealTimePublisher<Message>::create("topic_a2b");
  if (!publisher_a2b.has_value()) {
    std::cerr << "publisher_a2b: " << publisher_a2b.error().message() << std::endl;
    exit(1);
  }
  std::this_thread::sleep_for(1s);
  auto subscriber_b2a = ipcpp::ps::RealTimeSubscriber<Message>::create("topic_b2a");
  if (!subscriber_b2a.has_value()) {
    std::cerr << "subscriber_b2a: " << subscriber_b2a.error().message() << std::endl;
    exit(1);
  }
  subscriber_b2a->subscribe();

  std::uint64_t sum = 0;
  std::uint64_t expected_sum = 0;

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    if (publisher_a2b->publish(i)) {
      std::cerr << "publisher_a2b: failed to publish data" << std::endl;
      std::this_thread::sleep_for(1s);
      exit(1);
    }
    auto message = subscriber_b2a->await_message();
    sum += message->data;
    expected_sum += i;
  }

  benchmark_stop_barrier.arrive_and_wait();
  std::cout << "expected sum: " << expected_sum << std::endl;
  std::cout << "actual sum  : " << sum << std::endl;
}

void b2a() {
  ipcpp::logging::set_level(ipcpp::logging::level::debug);
  auto publisher_b2a = ipcpp::ps::RealTimePublisher<Message>::create("topic_b2a");
  if (!publisher_b2a.has_value()) {
    std::cerr << "publisher_b2a: " << publisher_b2a.error().message() << std::endl;
    exit(1);
  }
  std::this_thread::sleep_for(1s);
  auto subscriber_a2b = ipcpp::ps::RealTimeSubscriber<Message>::create("topic_a2b");
  if (!subscriber_a2b.has_value()) {
    std::cerr << "subscriber_a2b: " << subscriber_a2b.error().message() << std::endl;
    exit(1);
  }
  subscriber_a2b->subscribe();

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    auto message = subscriber_a2b->await_message();
    if (publisher_b2a->publish(i)) {
      std::cerr << "publisher_b2a: failed to publish data" << std::endl;
      std::this_thread::sleep_for(1s);
      exit(1);
    }
  }

  benchmark_stop_barrier.arrive_and_wait();
}

int main() {
  std::thread notifier(a2b);
  std::thread observer(b2a);

  sync_point.arrive_and_wait();
  auto start = ipcpp::utils::timestamp();
  benchmark_start_barrier.arrive_and_wait();

  benchmark_stop_barrier.arrive_and_wait();

  auto stop = ipcpp::utils::timestamp();
  
  notifier.join();
  observer.join();

  std::cout << "Iterations: " << num_iterations << "\n"
            << "Time:       " << stop - start << "ns\n"
            << "Latency:    " << static_cast<double>(stop - start) / static_cast<double>(2 * num_iterations) << "ns"
            << std::endl;

  return 0;
}