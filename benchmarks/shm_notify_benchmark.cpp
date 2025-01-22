/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of carry.
 */
#include <ipcpp/event/shm_atomic_notifier.h>
#include <ipcpp/event/shm_atomic_observer.h>
#include <ipcpp/utils/utils.h>

#include <barrier>
#include <chrono>
#include <numeric>
#include <thread>

using namespace std::chrono_literals;

std::barrier sync_point(3);
std::barrier benchmark_start_barrier(3);

constexpr auto num_iterations = 10000000;

void a2b() {
  auto notifier_a2b = carry::event::ShmAtomicNotifier::create("event_a2b");
  if (!notifier_a2b.has_value()) {
    std::cerr << "notifier_a2b: " << notifier_a2b.error().message() << std::endl;
    exit(1);
  }
  auto observer_b2a = carry::event::ShmAtomicObserver::create("event_b2a");
  if (!observer_b2a.has_value()) {
    std::cerr << "observer_b2a: " << observer_b2a.error().message() << std::endl;
    exit(1);
  }

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    notifier_a2b->notify_observers();
    observer_b2a->receive();
  }
}

void b2a() {
  auto notifier_b2a = carry::event::ShmAtomicNotifier::create("event_b2a");
  if (!notifier_b2a.has_value()) {
    std::cerr << "notifier_b2a: " << notifier_b2a.error().message() << std::endl;
    exit(1);
  }
  auto observer_a2b = carry::event::ShmAtomicObserver::create("event_a2b");
  if (!observer_a2b.has_value()) {
    std::cerr << "observer_a2b: " << observer_a2b.error().message() << std::endl;
    exit(1);
  }

  sync_point.arrive_and_wait();
  benchmark_start_barrier.arrive_and_wait();

  for (std::size_t i = 0; i < num_iterations; ++i) {
    observer_a2b->receive();
    notifier_b2a->notify_observers();
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