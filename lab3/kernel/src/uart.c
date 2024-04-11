#include "gpio.h"
#include "exception.h"
#include "uart.h"

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

#define IRQs1 ((volatile unsigned int *)(MMIO_BASE + 0x0000B210))
#define GPU_pend_1 ((volatile unsigned int *)(MMIO_BASE + 0x0000B204))

char read_buf[MAX_SIZE];
char write_buf[MAX_SIZE];
int read_front = 0;
int read_back = 0;
int write_front = 0;
int write_back = 0;

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

char uart_read()
{
    char c;
    while (1)
    {
        if (*AUX_MU_LSR & 1)
            break;
    }
    c = (char)(*AUX_MU_IO);

    return c == '\r' ? '\n' : c;
}

void uart_write(unsigned int c)
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
    {
        if (*s == '\n')
            uart_write('\r');
        uart_write(*s++);
    }
}

void uart_hex_upper_case(unsigned int d)
{
    for (int c = 28; c >= 0; c -= 4)
    {
        unsigned int n = (d >> c) & 0xF;
        n += n > 9 ? 55 : 48; // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        uart_write(n);
    }
}

void uart_hex_lower_case(unsigned int d)
{
    for (int c = 28; c >= 0; c -= 4)
    {
        unsigned int n = (d >> c) & 0xF;
        n += n > 9 ? 87 : 48; // 0-9 => '0'-'9', 10-15 => 'a'-'f'
        uart_write(n);
    }
}

void uart_dec(unsigned long long d)
{
    char reverse[100];
    int count = 0;
    if(d == 0)
    {
        uart_write('0');
        return;
    }

    while (d > 0)
    {
        reverse[count] = d % 10 + 48;
        count++;
        d /= 10;
    }
    for (int i = count - 1; i >= 0; i--)
        uart_write(reverse[i]);
}

void enable_uart_rx_interrupt() // When there is at least one byte data in rx FIFO, then interrupt.
{
    *AUX_MU_IER |= 1;
}

void disable_uart_rx_interrupt()
{
    *AUX_MU_IER &= ~1;
}

void enable_uart_tx_interrupt() // When tx FIFO is empty, then interrupt.
{
    *AUX_MU_IER |= 1 << 1;
}

void disable_uart_tx_interrupt()
{
    *AUX_MU_IER &= ~(1 << 1);
}

void enable_aux_interrupt()
{
    *IRQs1 |= 0b1 << 29;
}

void disable_aux_interrupt()
{
    *IRQs1 &= ~(0b1 << 29);
}

char uart_asyn_read()
{
    enable_uart_rx_interrupt();
    while (read_front == read_back); // if the read buffer is empty, then the process wait.

    disable_uart_rx_interrupt();
    char c = read_buf[read_front];
    read_front = (read_front + 1) % MAX_SIZE;
    enable_uart_rx_interrupt();

    return c == '\r' ? '\n' : c;
}

void uart_asyn_write(unsigned int c)
{
    if ((write_back + 1) % MAX_SIZE == write_front) // if the write buffer is full, drop it down.
        return;

    disable_uart_tx_interrupt();
    write_buf[write_back] = c;
    write_back = (write_back + 1) % MAX_SIZE;
    enable_uart_tx_interrupt();
}

void uart_asyn_puts(char *s)
{
    while (*s != '\0')
    {
        if (*s == '\n')
            uart_asyn_write('\r');
        uart_asyn_write(*s++);
    }
}

void uart_asyn_hex_lower_case(unsigned int d)
{
    for (int c = 28; c >= 0; c -= 4)
    {
        unsigned int n = (d >> c) & 0xF;
        n += n > 9 ? 87 : 48; // 0-9 => '0'-'9', 10-15 => 'a'-'f'
        uart_asyn_write(n);
    }
}

void uart_asyn_dec(unsigned int d)
{
    char reverse[100];
    int count = 0;
    while (d > 0)
    {
        reverse[count] = d % 10 + 48;
        count++;
        d /= 10;
    }
    for (int i = count - 1; i >= 0; i--)
        uart_asyn_write(reverse[i]);
}
