/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */
#include <ipcpp/publish_subscribe/fifo_publisher.h>
#include <ipcpp/publish_subscribe/fifo_subscriber.h>
#include <ipcpp/publish_subscribe/real_time_publisher.h>
#include <ipcpp/utils/utils.h>

#include <barrier>
#include <chrono>
#include <numeric>
#include <thread>

using namespace std::chrono_literals;

std::barrier sync_point(3);
std::barrier benchmark_start_barrier(3);

constexpr auto num_iterations = 1000000;

struct Message {
  std::int64_t msg_id;
};

void a2b() {
  auto publisher_a2b = carry::publish_subscribe::Publisher<Message>::create("topic_a2b");
  if (!publisher_a2b.has_value()) {
    std::cerr << "publisher_a2b: " << publisher_a2b.error().message() << std::endl;
    exit(1);
  }
  std::this_thread::sleep_for(1s);
  auto subscriber_b2a = carry::publish_subscribe::Subscriber<Message>::create("topic_b2a");
  if (!subscriber_b2a.has_value()) {
    std::cerr << "subscriber_b2a: " << subscriber_b2a.error().message() << std::endl;
    exit(1);
  }
  subscriber_b2a->subscribe();

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    publisher_a2b->publish(i);
    subscriber_b2a->receive([](const Message& msg) { return std::error_code(); });
  }
}

void b2a() {
  carry::logging::set_level(carry::logging::level::debug);
  auto publisher_b2a = carry::publish_subscribe::Publisher<Message>::create("topic_b2a");
  if (!publisher_b2a.has_value()) {
    std::cerr << "publisher_b2a: " << publisher_b2a.error().message() << std::endl;
    exit(1);
  }
  std::this_thread::sleep_for(1s);
  auto subscriber_a2b = carry::publish_subscribe::Subscriber<Message>::create("topic_a2b");
  if (!subscriber_a2b.has_value()) {
    std::cerr << "subscriber_a2b: " << subscriber_a2b.error().message() << std::endl;
    exit(1);
  }
  subscriber_a2b->subscribe();

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    subscriber_a2b->receive([](const Message& msg) { return std::error_code(); });
    publisher_b2a->publish(i);
  }
}

int main() {
  std::thread notifier(a2b);
  std::thread observer(b2a);

  sync_point.arrive_and_wait();
  auto start = carry::utils::timestamp();
  benchmark_start_barrier.arrive_and_wait();

  notifier.join();
  observer.join();

  auto stop = carry::utils::timestamp();

  std::cout << "Iterations: " << num_iterations << "\n"
            << "Time:       " << stop - start << "ns\n"
            << "Latency:    " << static_cast<double>(stop - start) / static_cast<double>(2 * num_iterations) << "ns"
            << std::endl;
  return 0;
}