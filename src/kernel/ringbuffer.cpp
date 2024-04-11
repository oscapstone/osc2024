#include "ringbuffer.hpp"

#include "util.hpp"

void RingBuffer::push(char c, bool wait) {
  if (not wait and full())
    return;
  while (full())
    NOP;
  buf[tail] = c;
  tail = ntail();
}

char RingBuffer::pop(bool wait) {
  if (not wait and empty())
    return 0;
  while (empty())
    NOP;
  auto c = buf[head];
  head = nhead();
  return c;
}
