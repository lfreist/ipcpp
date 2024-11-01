/**
 * Copyright 2024, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of ipcpp.
 */

#pragma once

#include <cstdint>

namespace ipcpp::utils::linked_list {

template <typename T>
class Node {
 public:
  explicit Node(T element) : _element(element) {};

  Node* insert(T element) {
    Node* new_node = new Node(element);
    new_node->_prev = this;
    if (_next != nullptr) {
      _next->_prev = new_node;
      new_node->_next = _next;
    }
    _next = new_node;
    return new_node;
  }

  Node* pop() {
    if (_prev != nullptr) {
      _prev->_next = _next;
    }
    if (_next != nullptr) {
      _next->_prev = _prev;
    }
    return this;
  }

  Node* next() const { return _next; }
  Node* prev() const { return _prev; }
  T data() const { return _element; }

 private:
  T _element;
  Node* _prev = nullptr;
  Node* _next = nullptr;
};

}
class DoubleLinkedList {};