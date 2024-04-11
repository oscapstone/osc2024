#include "mbox.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"
#include "peripherals/mmio.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile unsigned int *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020

unsigned int get(volatile unsigned int *addr) { return *addr; }

void set(volatile unsigned int *addr, unsigned int val) { *addr = val; }

void wait_cycles(int r) {
  if (r > 0) {
    while (r--) {
      asm volatile("nop");  // Execute the 'nop' instruction
    }
  }
}

void wait_usec(unsigned int n) {
  register unsigned long f, t, r;
  // get the current counter frequency
  asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
  // read the current counter
  asm volatile("mrs %0, cntpct_el0" : "=r"(t));
  // calculate required count increase
  unsigned long i = ((f / 1000) * n) / 1000;
  // loop while counter increase is less than i
  do {
    asm volatile("mrs %0, cntpct_el0" : "=r"(r));
  } while (r - t < i);
}