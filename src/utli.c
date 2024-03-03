#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *)0x3F10001c)
#define PM_WDOG ((volatile unsigned int *)0x3F100024)

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

void wait_cycle(int r) {
    if (r > 0) {
        while (r--) {
            asm volatile("nop"); // Execute the 'nop' instruction
        }
    }
}

/**
 * Wait N microsec (ARM CPU only)
 */
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