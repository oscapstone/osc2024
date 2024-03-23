#include "uart.h"
#include "irq.h"
#include "string.h"

static int uart_read_idx = 0;
static int uart_write_idx = 0;
static char uart_read_buffer[UART_BUF_SIZE];
static char uart_write_buffer[UART_BUF_SIZE];

void uart_init()
{
    // Configure GPIO pins
    register unsigned int r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |= (2 << 12) | (2 << 15);
    *GPFSEL1 = r;

    // Disable GPIO pull up/down
    *GPPUD = 0;
    for (int i = 0; i < 150; i++)
        ;
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    for (int i = 0; i < 150; i++)
        ;
    *GPPUD = 0;
    *GPPUDCLK0 = 0;

    // Initialize mini UART
    *AUX_ENABLE |= 1;   // Enable mini UART
    *AUX_MU_CNTL = 0;   // Disable Tx and Rx during setup
    *AUX_MU_IER = 0;    // Disable interrupt
    *AUX_MU_LCR = 3;    // Set data size to 8 bits
    *AUX_MU_MCR = 0;    // Disable auto flow control
    *AUX_MU_BAUD = 270; // Set baud rate to 115200
    *AUX_MU_IIR = 6;    // No FIFO
    *AUX_MU_CNTL = 3;   // Enable Tx and Rx

    // Enable AUX interrupts
    *ENABLE_IRQS_1 |= 1 << 29;
}

char uart_getc()
{
    // Check the data ready field on bit 0 of AUX_MU_LSR_REG
    while (!(*AUX_MU_LSR & 0x01))
        ;
    char c = (char)(*AUX_MU_IO); // Read from AUX_MU_IO_REG
    return c == '\r' ? '\n' : c;
}

void uart_putc(char c)
{
    // Convert '\n' to '\r\n'
    if (c == '\n')
        uart_putc('\r');

    // Check the transmitter empty field on bit 5 of AUX_MU_LSR_REG
    while (!(*AUX_MU_LSR & 0x20))
        ;
    *AUX_MU_IO = c; // Write to AUX_MU_IO_REG
}

void uart_hex(unsigned int h)
{
    uart_puts("0x");
    unsigned int n;
    for (int c = 28; c >= 0; c -= 4) {
        n = (h >> c) & 0xF;
        n += n > 9 ? 0x37 : '0';
        uart_putc(n);
    }
}

void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_enable_tx_interrupt()
{
    *AUX_MU_IER |= 0x02;
}

void uart_disable_tx_interrupt()
{
    *AUX_MU_IER &= 0x01;
}

void uart_enable_rx_interrupt()
{
    *AUX_MU_IER |= 0x01;
}

void uart_disable_rx_interrupt()
{
    *AUX_MU_IER &= 0x2;
}

void uart_tx_irq_handler()
{
    uart_disable_tx_interrupt();
    if (uart_write_idx < UART_BUF_SIZE &&
        uart_write_buffer[uart_write_idx] != 0) {
        // Buffer is not empty -> send characters
        *AUX_MU_IO = uart_write_buffer[uart_write_idx++];
        uart_enable_tx_interrupt();
    }
}

void uart_rx_irq_handler()
{
    char c = (char)(*AUX_MU_IO);
    uart_read_buffer[uart_read_idx++] = c == '\r' ? '\n' : c;
    if (uart_read_idx >= UART_BUF_SIZE)
        uart_read_idx = 0;
    uart_enable_rx_interrupt();
}

void uart_async_read(char *buf, int len)
{
    uart_disable_rx_interrupt();
    for (int i = 0; i < uart_read_idx && i < len; i++)
        buf[i] = uart_read_buffer[i];
    uart_read_idx = 0;
    uart_enable_rx_interrupt();
}

void uart_async_write(const char *s)
{
    int len = strlen(s);
    if (len >= UART_BUF_SIZE) {
        uart_puts("[ERROR] Exceed the UART buffer size.\n");
        return;
    }
    // Copy string to the write buffer
    for (int i = 0; i < len; i++)
        uart_write_buffer[i] = s[i];
    uart_write_buffer[len] = '\0';
    uart_write_idx = 0; // Reset the buffer index
    uart_enable_tx_interrupt();
}