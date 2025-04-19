/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <thread>
#include <chrono>
#include <expected>
#include <optional>
#include <format>

using namespace std::chrono_literals;

struct Data {
  int a[4196];
  std::int64_t timestamp;
};

int main(int argc, char** argv) {
  // creates:
  //  -
  auto ps_ws = carry::ps::service::open<Data, carry::delivery_mode::real_time>("topic_name", {.max_subscribers=8, .max_publishers=8, .history_size=0});
  std::expected<...> publisher = ps_ws.new_publisher();
  std::expected<...> subscriber = ps_ws.new_subscriber();

  std::thread sub_t([&]() {
    subscriber.subscribe();
    std::uint64_t msg_counter = 0;
    while (true) {
      std::optional<...> data = subscriber.await_message(100ms);
      std::int64_t timestamp = carry::utils::timestamp();
      if (data.has_data()) {
        std::println("{}ns", data->timestamp - timestamp);
      } else {
        std::println("Received {} messages", msg_counter);
        return;
      }
      msg_counter++;
    }
  });

  std::thread pub_t([&](){
    for (int i = 0; i < 100; ++i) {
      publisher->publish({.a={0}, .timestamp=carry::utils::timestamp()});
      std::this_thread::sleep_for(10ms);
    }
  });
}