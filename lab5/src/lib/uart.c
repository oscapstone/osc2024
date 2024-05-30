#include "uart.h"
#include "exception.h"
#include "gpio.h"
#include "string.h"

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

#define ENABLE_IRQS_1 ((volatile unsigned int *)(MMIO_BASE + 0xB210))
#define DISABLE_IRQS_1 ((volatile unsigned int *)(MMIO_BASE + 0xB21C))
#define BUFFER_SIZE 1024
#define VSPRINTF_BUF_SIZE 256

char uart_tx_buffer[BUFFER_SIZE];
char uart_rx_buffer[BUFFER_SIZE];
unsigned int uart_tx_buffer_head = 0;
unsigned int uart_tx_buffer_tail = 1;
unsigned int uart_rx_buffer_head = 0;
unsigned int uart_rx_buffer_tail = 1;

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

char uart_getrawc()
{
    char r;
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carrige return to newline */
    return r;
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

void uart_dec(unsigned int d)
{
    if (d == 0) {
        uart_send('0');
        return;
    }

    char buf[10];
    int i = 0;
    while (d > 0) {
        buf[i++] = d % 10 + '0';
        d /= 10;
    }
    for (int j = i - 1; j >= 0; j--)
        uart_send(buf[j]);

    // uart_puts("\n");
}

int uart_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[VSPRINTF_BUF_SIZE];

    char *str = (char *)buf;
    int count = vsprintf(str, fmt, args);
    while (*str) {
        if (*str == '\n')
            uart_send('\r');
        uart_send(*str++);
    }
    __builtin_va_end(args);
    return count;
}

void uart_interrupt_enable()
{
    // ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (p. 12)
    *AUX_MU_IER |= 0x1;        // enable rx interrupt
    *AUX_MU_IER |= 0x2;        // enable tx interrupt
    *ENABLE_IRQS_1 |= 1 << 29; // enable mini uart interrupt
}

void uart_interrupt_disable()
{
    *AUX_MU_IER &= ~0x1;          // disable rx interrupt
    *AUX_MU_IER &= ~0x2;          // disable tx interrupt
    *ENABLE_IRQS_1 &= ~(1 << 29); // disable mini uart interrupt
}

void uart_rx_interrupt_enable() { *AUX_MU_IER |= 0x1; }

void uart_rx_interrupt_disable() { *AUX_MU_IER &= ~0x1; }

void uart_tx_interrupt_enable() { *AUX_MU_IER |= 0x2; }

void uart_tx_interrupt_disable() { *AUX_MU_IER &= ~0x2; }

void uart_async_handler()
{
    if (*AUX_MU_IIR & 0x4) { // rx
        uart_rx_interrupt_disable();
        uart_rx_handler();
    }
    else if (*AUX_MU_IIR & 0x2) { // tx
        uart_tx_interrupt_disable();
        uart_tx_handler();
    }
}

void uart_rx_handler()
{
    if ((uart_rx_buffer_head + 1) % BUFFER_SIZE == uart_rx_buffer_tail) {
        uart_rx_interrupt_disable(); // disable read interrupt
        return;
    }

    uart_rx_buffer[uart_rx_buffer_head] = (char)(*AUX_MU_IO);
    uart_rx_buffer_head = (uart_rx_buffer_head + 1) % BUFFER_SIZE;
    // uart_rx_interrupt_enable();
}

void uart_tx_handler()
{
    if (uart_tx_buffer_head == uart_tx_buffer_tail) {
        uart_tx_interrupt_disable(); // disable write interrupt
        return;
    }

    asm volatile("msr DAIFSet, 0xf");
    uart_send(uart_tx_buffer[uart_tx_buffer_tail]);
    uart_tx_buffer_tail = (uart_tx_buffer_tail + 1) % BUFFER_SIZE;
    uart_tx_interrupt_enable();
    asm volatile("msr DAIFClr, 0xf");
}

void uart_async_puts(char *s)
{
    while ((uart_tx_buffer_head + 1) % BUFFER_SIZE == uart_tx_buffer_tail)
        uart_tx_interrupt_enable();

    asm volatile("msr DAIFSet, 0xf");
    while (*s) {
        uart_tx_buffer[uart_tx_buffer_head++] = *s++;
        uart_tx_buffer_head %= BUFFER_SIZE;
    }
    asm volatile("msr DAIFClr, 0xf");

    uart_tx_interrupt_enable();
}

void uart_async_putc(char c)
{
    while ((uart_tx_buffer_head + 1) % BUFFER_SIZE == uart_tx_buffer_tail) {
        uart_tx_interrupt_enable();
    }

    lock();
    uart_tx_buffer[uart_tx_buffer_head++] = c;
    uart_tx_buffer_head %= BUFFER_SIZE;
    unlock();

    uart_tx_interrupt_enable();
}

char uart_async_getc()
{
    uart_rx_interrupt_enable();
    while ((uart_rx_buffer_tail == uart_rx_buffer_head))
        uart_rx_interrupt_enable();

    lock();
    char c = uart_rx_buffer[uart_rx_buffer_tail]; // get a byte from rx buffer
    uart_rx_buffer_tail = (uart_rx_buffer_tail + 1) % BUFFER_SIZE;
    unlock();

    return c;
}

void uart_clear_buffers()
{
    for (int i = 0; i < BUFFER_SIZE; i++) {
        uart_rx_buffer[i] = 0;
        uart_tx_buffer[i] = 0;
    }
}