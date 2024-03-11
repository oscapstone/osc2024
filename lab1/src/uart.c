#include "uart.h"
#include "gpio.h"

/*
 * Auxilary mini UART registers
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (p. 8)
 */
#define AUX_BASE (MMIO_BASE + 0x215000)
#define AUX_ENABLE ((volatile unsigned int *)(AUX_BASE + 0x04))
#define AUX_MU_IO ((volatile unsigned int *)(AUX_BASE + 0x40))
#define AUX_MU_IER ((volatile unsigned int *)(AUX_BASE + 0x44))
#define AUX_MU_IIR ((volatile unsigned int *)(AUX_BASE + 0x48))
#define AUX_MU_LCR ((volatile unsigned int *)(AUX_BASE + 0x4C))
#define AUX_MU_MCR ((volatile unsigned int *)(AUX_BASE + 0x50))
#define AUX_MU_LSR ((volatile unsigned int *)(AUX_BASE + 0x54))
#define AUX_MU_MSR ((volatile unsigned int *)(AUX_BASE + 0x58))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(AUX_BASE + 0x5C))
#define AUX_MU_CNTL ((volatile unsigned int *)(AUX_BASE + 0x60))
#define AUX_MU_STAT ((volatile unsigned int *)(AUX_BASE + 0x64))
#define AUX_MU_BAUD ((volatile unsigned int *)(AUX_BASE + 0x68))

void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1; // enable UART1, AUX mini UART
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xC6; // disable interrupts
    *AUX_MU_BAUD = 270; // 115200 baud

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0; // enable pins 14 and 15
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;   // flush GPIO setup
    *AUX_MU_CNTL = 3; // enable Tx, Rx
}

void uart_send(unsigned int c)
{
    /* wait until we can send */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));

    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

char uart_getc()
{
    char r;
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carrige return to newline */
    return r == '\r' ? '\n' : r;
}

void uart_puts(char *s)
{
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n') {
            uart_send('\r');
        }
        uart_send(*s++);
    }
}

void uart_hex(unsigned int d)
{
    uart_puts("0x");
    unsigned int n;
    for (int c = 28; c >= 0; c -= 4) {
        n = (d >> c) & 0xF;
        n += n > 9 ? 0x57 : 0x30;
        uart_send(n);
    }
    uart_puts("\n");
}