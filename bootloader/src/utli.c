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

float get_timestamp() {
    register unsigned long long f, c;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(f)); // get current counter frequency
    asm volatile("mrs %0, cntpct_el0" : "=r"(c)); // read current counter
    return (float)c / f;
}

unsigned int get(volatile unsigned int *addr) {
    return *addr;
}

void set(volatile unsigned int *addr, unsigned int val) {
    *addr = val;
}

void reset(int tick) {                // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20); // full reset
    set(PM_WDOG, PM_PASSWORD | tick); // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0); // full reset
    set(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
}

void wait_cycles(int r) {
    if (r > 0) {
        while (r--) {
            asm volatile("nop"); // Execute the 'nop' instruction
        }
    }
}