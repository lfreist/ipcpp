/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
* Author: Leon Freist <freist.leon@gmail.com>
*
* This file is part of ipcpp.
*/

#include <ipcpp/datatypes/fixed_size_stack.h>

using namespace ipcpp;

int main(int argc, char** argv) {
 int data[100];
 auto result = FixedSizeStack<int>::create(10, config::QueueFullPolicy::ERROR,
                                           std::span<uint8_t>(reinterpret_cast<uint8_t*>(data), 100 * sizeof(int)));
 if (result.has_value()) {
   auto& q = result.value();
   for (int i = 0; i < 10; ++i) {
     q.push(i);
     std::cout << i << ", " << q.size() << std::endl;
   }
   for (int i = 0; i < 5; ++i) {
     std::cout << q.front() << std::endl;
     q.pop();
     std::cout << i << ", " << q.size() << std::endl;
   }
   for (int i = 0; i < 4; ++i) {
     q.push(i);
     std::cout << i << ", " << q.size() << std::endl;
   }
   std::cout << "---" << std::endl;
   auto result2 = FixedSizeStack<int>::create(std::span<uint8_t>(reinterpret_cast<uint8_t*>(data), 100 * sizeof(int)));
   if (result.has_value()) {
     while (!q.empty()) {
       std::cout << q.front() << std::endl;
       q.pop();
       std::cout << "size: " << q.size() << std::endl;
     }
   }
 } else {
   std::cerr << "error" << std::endl;
   return 1;
 }
}