#include "utils.h"

void wait_cycles(int c) {
  while (c--) {
    asm volatile("nop");
  }
}
