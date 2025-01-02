/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#include <ipcpp/publish_subscribe/message.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, char** argv) {
  ipcpp::ps::Message<std::vector<int>> message;

  std::thread a([&]() {
    {
      auto write_access = message.request_writable();
      if (write_access) {
        std::cout << "Acquired write access" << std::endl;
        std::cout << "sleeping 5 seconds" << std::endl;
        std::this_thread::sleep_for(5s);
        write_access->emplace(1, 1, std::vector<int>(10));
      } else {
        std::cout << "Failed to acquire write access" << std::endl;
      }
    }
    std::cout << "released write access" << std::endl;
    std::this_thread::sleep_for(2s);
    if (!message.request_writable()) {
      std::cout << "failed to acquire write access" << std::endl;
    }
  });

  std::thread b([&]() {
    while (true) {
      auto read_access = message.consume();
      if (read_access) {
        std::cout << "Acquired read access" << std::endl;
        std::cout << read_access.value()->size() << std::endl;
        std::this_thread::sleep_for(2s);
        std::cout << "done" << std::endl;
        break;
      }
      std::cout << "failed to acquire read access" << std::endl;
      std::this_thread::sleep_for(1s);
    }
  });

  a.join();
  b.join();
}