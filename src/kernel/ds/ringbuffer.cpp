#include "ds/ringbuffer.hpp"

#include "int/interrupt.hpp"
#include "sched.hpp"

void RingBuffer::push(char c, bool wait) {
  if (not wait and full())
    return;
  while (full())
    schedule();
  save_DAIF_disable_interrupt();
  buf[tail] = c;
  tail = ntail();
  restore_DAIF();
}

char RingBuffer::pop(bool wait) {
  if (not wait and empty())
    return -1;
  while (empty())
    schedule();
  save_DAIF_disable_interrupt();
  auto c = buf[head];
  head = nhead();
  restore_DAIF();
  return c;
}
