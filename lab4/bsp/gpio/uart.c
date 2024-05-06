#include <kernel/bsp_port/uart.h>
#include <lib/string.h>

#include "../irq/base.h"  // for enable uart irq
#include "gpio.h"

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

void uart_init() {
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1;    // Enable mini UART0x01
    *AUX_MU_CNTL = 0;    // Disable TX, RX during configuration
    *AUX_MU_IER = 0;     // Disable interrupt
    *AUX_MU_LCR = 3;     // Set the data size to 8 bit
    *AUX_MU_MCR = 0;     // Don't need auto flow control
    *AUX_MU_BAUD = 270;  // Set baud rate to 115200
    *AUX_MU_IIR = 6;     // No FIFO

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));  // gpio14, gpio15 innitial
    r |= (2 << 12) | (2 << 15);     // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;  // enable pins 14 and 15
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;    // flush GPIO setup
    *AUX_MU_CNTL = 3;  // enable Tx, Rx // bit1:transmitter bit0:receiver

    *ENABLE_IRQS_1 |= (1 << 29);
}

void uart_send(unsigned int c) {
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));  // This bit is set if the transmit FIFO
                                      // can accept at least one byte.
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

char uart_recv() {
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));  // This bit is set if the receive FIFO
                                      // holds at least 1 symbol.
    return (char)(*AUX_MU_IO);
}

char uart_getc() {
    char r;
    do {
        asm volatile("nop");
    } while (!(
        *AUX_MU_LSR &
        0x01));  // This bit is set if the receive FIFO holds at least 1 symbol.
    
    r = (char)(*AUX_MU_IO);
    // convert carrige return to newline
    return r == '\r' ? '\n' : r;
}

void uart_puts(const char *s) {
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n') uart_send('\r');
        uart_send(*s++);
    }
}

void uart_hex(unsigned long long int d) {
    uart_puts("0x");
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

void uart_enable_tx_interrupt() {
#ifdef DEBUG
    uart_puts("[INFO] enable tx interrupt\n");
#endif /* ifdef DEBUG */

    *AUX_MU_IER |= (1 << 1);
}

void uart_disable_tx_interrupt() {
#ifdef DEBUG
    uart_puts("[INFO] disable tx interrupt\n");
#endif /* ifdef DEBUG */
    *AUX_MU_IER &= ~(1 << 1);
}

void uart_enable_rx_interrupt() {
#ifdef DEBUG
    uart_puts("[INFO] enable rx interrupt\n");
#endif /* ifdef DEBUG */
    *AUX_MU_IER |= (1 << 0);
}

void uart_disable_rx_interrupt() {
#ifdef DEBUG
    uart_puts("[INFO] disable rx interrupt\n");
#endif /* ifdef DEBUG */
    *AUX_MU_IER &= ~(1 << 0);
}

#define UART_BUF_SIZE 2048
static char write_buffer[UART_BUF_SIZE];
static char read_buffer[UART_BUF_SIZE];
static int write_idx = 0, write_end = 0;
static int read_idx = 0;

void uart_rx_irq_handler() {
    char c = (char)(*AUX_MU_IO);
    read_buffer[read_idx++] = c == '\r' ? '\n' : c;
    read_buffer[read_idx] = '\0';
    if (read_idx >= UART_BUF_SIZE) read_idx = 0;
    uart_enable_rx_interrupt();
}

void uart_tx_irq_handler() {
    uart_disable_tx_interrupt();
    if (write_idx != write_end && write_buffer[write_idx] != '\0') {
        *AUX_MU_IO = write_buffer[write_idx];
        write_idx = (write_idx + 1) % UART_BUF_SIZE;
        uart_enable_tx_interrupt();
    }
}

void uart_async_read(char *buf) {
    uart_disable_rx_interrupt();
    strncpy(buf, read_buffer, read_idx);
    buf[read_idx] = '\0';
    read_idx = 0;
}

void uart_async_write(const char *s) {
    // append string to write buffer
    while (*s) {
        write_buffer[write_end] = *s++;
        write_end = (write_end + 1) % UART_BUF_SIZE;
    }
    write_buffer[write_end] = '\0';
    uart_enable_tx_interrupt();
}
