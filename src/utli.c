#include "mbox.h"
#include "my_math.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"
#include "peripherals/mmio.h"
#include "uart0.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC ((volatile unsigned int *)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile unsigned int *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020

char *itox(int value, char *s) {
    int idx = 0;
    unsigned int n;
    for (int c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (value >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        s[idx++] = n;
    }
    s[idx] = '\0';
    return s;
}

char *itoa(int value, char *s) {
    int idx = 0;
    if (value < 0) {
        value *= -1;
        s[idx++] = '-';
    }

    char tmp[10];
    int tidx = 0;
    do {
        tmp[tidx++] = '0' + value % 10;
        value /= 10;
    } while (value != 0 && tidx < 11);

    // reverse tmp
    int i;
    for (i = tidx - 1; i >= 0; i--) {
        s[idx++] = tmp[i];
    }
    s[idx] = '\0';

    return s;
}

char *ftoa(float value, char *s) {
    int idx = 0;
    if (value < 0) {
        value = -value;
        s[idx++] = '-';
    }

    int ipart = (int)value;
    float fpart = value - (float)ipart;

    // convert ipart
    char istr[11]; // 10 digit
    itoa(ipart, istr);

    // convert fpart
    char fstr[7]; // 6 digit
    fpart *= pow(10, 6);
    itoa((int)fpart, fstr);

    // copy int part
    char *ptr = istr;
    while (*ptr)
        s[idx++] = *ptr++;
    s[idx++] = '.';
    // copy float part
    ptr = fstr;
    while (*ptr)
        s[idx++] = *ptr++;
    s[idx] = '\0';

    return s;
}

void align(void *size, unsigned int s) {
    unsigned int *x = (unsigned int *)size;
    unsigned int mask = s - 1;
    *x = ((*x) + mask) & (~mask);
}

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

/**
 * Shutdown the board
 */
void power_off() {
    unsigned long r;

    // power off devices one by one
    for (r = 0; r < 16; r++) {
        mbox[0] = 8 * 4;
        mbox[1] = MBOX_CODE_BUF_REQ;
        mbox[2] = MBOX_TAG_SET_POWER; // set power state
        mbox[3] = 8;
        mbox[4] = MBOX_CODE_TAG_REQ;
        mbox[5] = (unsigned int)r; // device id
        mbox[6] = 0;               // bit 0: off, bit 1: no wait
        mbox[7] = MBOX_TAG_LAST;
        mbox_call(MBOX_CH_PROP);
    }

    // power off gpio pins (but not VCC pins)
    set(GPFSEL0, 0);
    set(GPFSEL1, 0);
    set(GPFSEL2, 0);
    set(GPFSEL3, 0);
    set(GPFSEL4, 0);
    set(GPFSEL5, 0);
    set(GPPUD, 0);

    wait_cycles(150);
    set(GPPUDCLK0, 0xffffffff);
    set(GPPUDCLK1, 0xffffffff);
    wait_cycles(150);
    set(GPPUDCLK0, 0);
    set(GPPUDCLK1, 0); // flush GPIO setup

    // power off the SoC (GPU + CPU)
    r = get(PM_RSTS);
    r &= ~0xfffffaaa;
    r |= 0x555; // partition 63 used to indicate halt
    set(PM_RSTS, PM_WDOG_MAGIC | r);
    set(PM_WDOG, PM_WDOG_MAGIC | 10);
    set(PM_RSTC, PM_WDOG_MAGIC | PM_RSTC_FULLRST);
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