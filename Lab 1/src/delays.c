
/**
 * Wait N CPU cycles (ARM CPU only)
 */
void wait_cycles(unsigned int n)
{
    if (n) while (n--) { asm volatile("nop"); }
}


/**
 * Wait N microseconds (ARM CPU only)
 */
void wait_msec(unsigned int n)
{
    register unsigned long freq, start, cur;

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));       // get current counter frequency
    asm volatile ("mrs %0, cntpct_el0" : "=r"(start));      // read the current counter

    unsigned long i = ((freq / 1000) * n) / 1000;           // calculate required count increase

    do {
        asm volatile ("mrs %0, cntpct_el0" : "=r"(cur));
    } while (cur - start < i);                              // loop while counter increase is less than i
}
