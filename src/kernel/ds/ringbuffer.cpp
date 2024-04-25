#include "ds/ringbuffer.hpp"

#include "int/interrupt.hpp"
#include "util.hpp"

void RingBuffer::push(char c, bool wait) {
  if (not wait and full())
    return;
  while (full())
    NOP;
  save_DAIF_disable_interrupt();
  buf[tail] = c;
  tail = ntail();
  restore_DAIF();
}

char RingBuffer::pop(bool wait) {
  if (not wait and empty())
    return -1;
  while (empty())
    NOP;
  save_DAIF_disable_interrupt();
  auto c = buf[head];
  head = nhead();
  restore_DAIF();
  return c;
}
