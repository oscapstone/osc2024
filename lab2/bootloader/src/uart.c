#include "gpio.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

void uart_init()
{
    unsigned int r;
    // set gpio14, gpio15 to alt5
    r = *GPFSEL1;
    r &= ~((7 << 15) | (7 << 12));
    r |= ((2 << 15) | (2 << 12));
    *GPFSEL1 = r;

    // disable pull-up/down of gpio14, gpio15
    *GPPUD = 0;
    int cycle = 150;
    while (cycle--) // set up time
        asm volatile("nop");
    *GPPUDCLK0 = ((1 << 15) || (1 << 14));
    cycle = 150;
    while (cycle--) // hold time
        asm volatile("nop");
    *GPPUDCLK0 = 0;

    *AUX_ENABLE |= 1;
    *AUX_MU_CNTL = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_LCR = 3;
    *AUX_MU_MCR = 0;
    *AUX_MU_BAUD = 270;
    *AUX_MU_IIR = 6;
    *AUX_MU_CNTL = 3;
}

unsigned char uart_rev()
{
    while (1)
    {
        if (*AUX_MU_LSR & 1)
            break;
    }
    return (unsigned char)(*AUX_MU_IO);
}

void uart_send(unsigned int c)
{
    while (1)
    {
        if (*AUX_MU_LSR & (1 << 5))
            break;
    }
    *AUX_MU_IO = c;
}

void uart_puts(char *s)
{
    while (*s != '\0')
        uart_send(*s++);
}