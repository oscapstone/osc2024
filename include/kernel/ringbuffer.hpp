#pragma once

class RingBuffer {
  static constexpr int capacity = 0x1000;
  int head = 0, tail = 0;
  char buf[capacity]{};

  int nhead() const {
    return (head + 1) % capacity;
  }
  int ntail() const {
    return (tail + 1) % capacity;
  }

 public:
  bool empty() const {
    return head == tail;
  }
  bool full() const {
    return ntail() == head;
  }
  int size() const {
    return tail - head + (head <= tail ? 0 : capacity);
  }
  void push(char c, bool wait);
  char pop(bool wait);
};
