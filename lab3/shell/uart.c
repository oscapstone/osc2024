#include "header/uart.h"

/*
 * Auxilary mini UART registers
 * ref: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf (p. 8)
 */
#define AUX_BASE (MMIO_BASE + 0x215000)
#define AUX_ENABLE ((volatile unsigned int *)(AUX_BASE + 0x04))
#define AUX_MU_IO ((volatile unsigned int *)(AUX_BASE + 0x40)) // for io data
#define AUX_MU_IER ((volatile unsigned int *)(AUX_BASE + 0x44))
#define AUX_MU_IIR ((volatile unsigned int *)(AUX_BASE + 0x48)) // for interrupt identify
#define AUX_MU_LCR ((volatile unsigned int *)(AUX_BASE + 0x4C))
#define AUX_MU_MCR ((volatile unsigned int *)(AUX_BASE + 0x50))
#define AUX_MU_LSR ((volatile unsigned int *)(AUX_BASE + 0x54))
#define AUX_MU_MSR ((volatile unsigned int *)(AUX_BASE + 0x58))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(AUX_BASE + 0x5C))
#define AUX_MU_CNTL ((volatile unsigned int *)(AUX_BASE + 0x60))
#define AUX_MU_STAT ((volatile unsigned int *)(AUX_BASE + 0x64))
#define AUX_MU_BAUD ((volatile unsigned int *)(AUX_BASE + 0x68))

//p112
#define ENABLE_IRQS_1 ((volatile unsigned int *)(MMIO_BASE + 0xB210))
#define DISABLE_IRQS_1 ((volatile unsigned int *)(MMIO_BASE + 0xB21C))
#define BUFFER_SIZE 1024
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

void uart_send_char(unsigned int c)
{
    /* wait until we can send */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));

    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

char uart_get_char()
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

void uart_send_str(char *s)
{
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n') {
            uart_send_char('\r');
        }
        uart_send_char(*s++);
    }
}

void uart_binary_to_hex(unsigned int d)
{
    uart_send_str("0x");
    unsigned int n;
    for (int c = 28; c >= 0; c -= 4) {
        n = (d >> c) & 0xF;
        n += n > 9 ? 0x57 : 0x30;
        uart_send_char(n);
    }
    uart_send_str("\n");
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

void uart_rx_handler()
{
    if ((uart_rx_buffer_head + 1) % BUFFER_SIZE == uart_rx_buffer_tail) { // checks if the next position in the receive buffer (uart_rx_buffer_head + 1) would overlap with the current tail of the buffer (uart_rx_buffer_tail).
        uart_rx_interrupt_disable(); // disable read interrupt
        return;
    }

    uart_rx_buffer[uart_rx_buffer_head] = (char)(*AUX_MU_IO); // receive data, reads a character from the UART receive register (*AUX_MU_IO) and stores it in the receive buffer at the current head position (uart_rx_buffer[uart_rx_buffer_head]).
    uart_rx_buffer_head = (uart_rx_buffer_head + 1) % BUFFER_SIZE; //Update Buffer Head Pointer: (uart_rx_buffer_head + 1) % BUFFER_SIZE ensures that the head pointer wraps around to the beginning of the buffer if it reaches the end (BUFFER_SIZE) to implement a circular buffer.
}

void uart_tx_handler()
{
    if (uart_tx_buffer_head == uart_tx_buffer_tail) { // check buffer is empty or not, if empty nothing to write
        uart_tx_interrupt_disable(); // disable write interrupt
        return;
    }

    asm volatile("msr DAIFSet, 0xf");
    uart_send_char(uart_tx_buffer[uart_tx_buffer_tail]); // send last character of writing buffer
    uart_tx_buffer_tail = (uart_tx_buffer_tail + 1) % BUFFER_SIZE; //  ensures that the tail pointer wraps around to the beginning of the buffer if it reaches the end (BUFFER_SIZE) to implement a circular buffer.
    asm volatile("msr DAIFClr, 0xf");
    uart_tx_interrupt_enable();
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
    while ((uart_tx_buffer_head + 1) % BUFFER_SIZE == uart_tx_buffer_tail) // check list is full or not by comparing head+1 & tail
        uart_tx_interrupt_enable(); // if not enable writing interrupt to enable writing handler start writing interrupt
    // below is
    asm volatile("msr DAIFSet, 0xf");
    uart_tx_buffer[uart_tx_buffer_head++] = c; // store c at current head position and increase one for next position
    uart_tx_buffer_head %= BUFFER_SIZE; // avoid index out of range
    asm volatile("msr DAIFClr, 0xf");

    uart_tx_interrupt_enable();
}

char uart_async_getc()
{
    while ((uart_rx_buffer_tail == uart_rx_buffer_head)) // check if reading buffer is empty
        uart_rx_interrupt_enable(); // if empty enable reading handler to start reading

    asm volatile("msr DAIFSet, 0xf");
    char c = uart_rx_buffer[uart_rx_buffer_tail]; // get a byte from rx buffer
    uart_rx_buffer_tail = (uart_rx_buffer_tail + 1) % BUFFER_SIZE; // avoid index out of range 
    asm volatile("msr DAIFClr, 0xf");

    return c;
}

void uart_clear_buffers()
{
    for (int i = 0; i < BUFFER_SIZE; i++) {
        uart_rx_buffer[i] = '\0';
        uart_tx_buffer[i] = '\0';
    }
}