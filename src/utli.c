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
    while (r--) {
        asm volatile("nop"); // Execute the 'nop' instruction
    }
}