#include "bcm2837/rpi_gpio.h"
#include "bcm2837/rpi_uart1.h"
#include "bcm2837/rpi_irq.h"
#include "uart1.h"
#include "stdio.h"
#include "exception.h"

// implement first in first out buffer with a read index and a write index
char uart_tx_buffer[VSPRINT_MAX_BUF_SIZE] = {0};
int uart_tx_buffer_widx = 0; // write index
int uart_tx_buffer_ridx = 0; // read index
char uart_rx_buffer[VSPRINT_MAX_BUF_SIZE] = {0};
int uart_rx_buffer_widx = 0;
int uart_rx_buffer_ridx = 0;

void uart_init()
{
    register unsigned int selector;

    /* initialize UART */
    *AUX_ENABLES = 1;     // enable UART1
    *AUX_MU_CNTL_REG = 0; // disable TX/RX

    /* configure UART */
    *AUX_MU_IER_REG = 0;    // disable interrupt
    *AUX_MU_LCR_REG = 3;    // 8 bit data size
    *AUX_MU_MCR_REG = 0;    // disable flow control
    *AUX_MU_BAUD_REG = 270; // 115200 baud rate
    *AUX_MU_IIR_REG = 6;    // clean FIFO

    /* map UART1 to GPIO pins */
    selector = *GPFSEL1;
    selector &= ~(7 << 12); // clean gpio14, and (11 111 111 111 111 111 000 111 111 111 111)2
    selector |= 2 << 12;    // set gpio14 to alt5
    selector &= ~(7 << 15); // clean gpio15, and (11 111 111 111 111 000 111 111 111 111 111)2
    selector |= 2 << 15;    // set gpio15 to alt5
    *GPFSEL1 = selector;

    /* enable pin 14, 15 - ref: Page 101 */
    *GPPUD = 0;
    selector = 150;
    while (selector--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    selector = 150;
    while (selector--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;

    *AUX_MU_CNTL_REG = 3; // enable TX/RX
}

void uart_flush_FIFO()
{
    // https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf Pg.13
    // On write:
    //  Writing with bit 1 set will clear the receive FIFO
    //  Writing with bit 2 set will clear the transmit FIFOF
    *AUX_MU_IIR_REG |= 6;
}

char uart_recv()
{
    char r;
    while (!(*AUX_MU_LSR_REG & 0x01))
        ;
    r = (char)(*AUX_MU_IO_REG);
    return r;
}

void uart_send(unsigned int c)
{
    while (!(*AUX_MU_LSR_REG & 0x20))
        ;
    *AUX_MU_IO_REG = c;
}

void uart_puts(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char tmp[VSPRINT_MAX_BUF_SIZE];
    char *s = tmp;
    // use sprintf to format our string
    vsprintf(s,fmt,args);
    while (*s)
        uart_send(*s++);
}

// uart_async_getc read from buffer
// uart_r_irq_handler write to buffer then output
char uart_async_recv()
{
    *AUX_MU_IER_REG |= 1; // enable read interrupt
    // do while if buffer empty
    lock_interrupt();
    while (uart_rx_buffer_ridx == uart_rx_buffer_widx)
    {
        unlock_interrupt();
        // DEBUG("uart_async_recv buffer empty\r\n");
        *AUX_MU_IER_REG |= 1; // enable read interrupt
        lock_interrupt();
    }
    char r = uart_rx_buffer[uart_rx_buffer_ridx++];
    if (uart_rx_buffer_ridx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_ridx = 0;
    unlock_interrupt();
    return r;
}

// uart_async_putc writes to buffer
// uart_w_irq_handler read from buffer then output
void uart_async_send(char c)
{
    lock_interrupt();
    // if buffer full, wait for uart_w_irq_handler
    while ((uart_tx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_tx_buffer_ridx)
    {
        unlock_interrupt();
        *AUX_MU_IER_REG |= 2; // enable write interrupt
        lock_interrupt();
    }
    uart_tx_buffer[uart_tx_buffer_widx++] = c;
    if (uart_tx_buffer_widx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_widx = 0; // cycle pointer
    unlock_interrupt();
    *AUX_MU_IER_REG |= 2; // enable write interrupt
}

int is_uart_rx_buffer_full()
{
    return (uart_rx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_rx_buffer_ridx;
}

int is_uart_tx_buffer_full()
{
    return (uart_tx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_tx_buffer_ridx;
}

// AUX_MU_IER_REG -> BCM2837-ARM-Peripherals.pdf - Pg.12
void uart_interrupt_enable()
{
    *AUX_MU_IER_REG |= 1;      // enable read interrupt
    *AUX_MU_IER_REG |= 2;      // enable write interrupt
    *ENABLE_IRQS_1 |= 1 << 29; // Pg.112
}

void uart_interrupt_disable()
{
    *AUX_MU_IER_REG &= ~(1); // disable read interrupt
    *AUX_MU_IER_REG &= ~(2); // disable write interrupt
}

void uart_r_irq_handler()
{
    lock_interrupt();
    // uart_puts("uart_r_irq_handler\r\n");
    if ((uart_rx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_rx_buffer_ridx)
    {
        *AUX_MU_IER_REG &= ~(1); // disable read interrupt
        unlock_interrupt();
        return;
    }
    unlock_interrupt();
    uart_rx_buffer[uart_rx_buffer_widx] = uart_recv();
    lock_interrupt();
    uart_rx_buffer_widx++;
    if (uart_rx_buffer_widx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_widx = 0;
    *AUX_MU_IER_REG |= 1;
    unlock_interrupt();
}

void uart_w_irq_handler()
{
    lock_interrupt();
    if (uart_tx_buffer_ridx == uart_tx_buffer_widx)
    {
        *AUX_MU_IER_REG &= ~(2); // disable write interrupt
        unlock_interrupt();
        return; // buffer empty
    }
    unlock_interrupt();
    uart_send(uart_tx_buffer[uart_tx_buffer_ridx]);
    lock_interrupt();
    uart_tx_buffer_ridx++;
    if (uart_tx_buffer_ridx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_ridx = 0;
    *AUX_MU_IER_REG |= 2; // enable write interrupt
    unlock_interrupt();
}