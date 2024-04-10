#include "interrupt.hpp"

void enable_interrupt() {
  asm("msr DAIFClr, 0xf");
}
void disable_interrupt() {
  asm("msr DAIFSet, 0xf");
}
