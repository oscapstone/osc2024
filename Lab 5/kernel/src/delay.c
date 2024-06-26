#include "type.h"

/* Wait N CPU cycles (ARM CPU only) */

void 
delay_cycles(uint32_t n)
{
    if (n) while (n--) { asm volatile("nop"); }
}

/**
 * Wait N microseconds (ARM CPU only)
 */
void 
delay_msec(uint32_t n)
{
    register uint64_t freq, start, cur;

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));       // get current counter frequency
    asm volatile ("mrs %0, cntpct_el0" : "=r"(start));      // read the current counter

    uint64_t i = ((freq / 1000) * n) / 1000;                // calculate required count increase

    do {
        asm volatile ("mrs %0, cntpct_el0" : "=r"(cur));
    } while (cur - start < i);                              // loop while counter increase is less than i
}