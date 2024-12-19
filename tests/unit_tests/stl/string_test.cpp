/**
* Copyright 2024, Leon Freist (https://github.com/lfreist)
* Author: Leon Freist <freist.leon@gmail.com>
*
* This file is part of ipcpp.
*/

#include <ipcpp/stl/vector.h>

#include <iostream>

template<typename Iterator>
void print(Iterator first, Iterator end) {
  std::cout << "{";
  for (; first != end; ++first) {
    std::cout << *first << ", ";
  }
  std::cout << "}" << std::endl;
}

int main() {
  int a[4096];
  ipcpp::pool_allocator<uint8_t>::initialize_factory(a, 4096);
  ipcpp::vector<int> vec {1, 2, 3, 4};
  print(vec.begin(), vec.end());
  vec.emplace_back(8);
  print(vec.begin(), vec.end());
  return 0;
}