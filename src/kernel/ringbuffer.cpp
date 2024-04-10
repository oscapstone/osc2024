#include "ringbuffer.hpp"

#include "util.hpp"

void RingBuffer::push(char c) {
  while (full())
    NOP;
  buf[tail] = c;
  tail = ntail();
}

char RingBuffer::pop() {
  while (empty())
    NOP;
  auto c = buf[head];
  head = nhead();
  return c;
}
