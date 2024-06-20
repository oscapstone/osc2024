#include "utils.h"
#include "mini_uart.h"
#include "exception.h"
#include "timer.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "peripherals/rpi_irq.h"

// Define uart TX/RX buffer
char    uart_tx_buffer[VSPRINT_MAX_BUF_SIZE] = {0};
int     uart_tx_buffer_r_idx = 0;
int     uart_tx_buffer_w_idx = 0;

char    uart_rx_buffer[VSPRINT_MAX_BUF_SIZE] = {0};
int     uart_rx_buffer_r_idx = 0;
int     uart_rx_buffer_w_idx = 0;

void uart_send(unsigned int c) {
    while(!(*AUX_MU_LSR_REG & 0x20)){};
    *AUX_MU_IO_REG = c;
}

char uart_getc() {
    char r;
    while(!(*AUX_MU_LSR_REG & 0x01)){};
    r = (char)(*AUX_MU_IO_REG);
    return r;
}

char uart_recv() {
	char r;
    while(!(*AUX_MU_LSR_REG & 0x01)){};
    r = (char)(*AUX_MU_IO_REG);
    return r=='\r'?'\n':r;
}

int uart_puts(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[VSPRINT_MAX_BUF_SIZE];

    char *str = (char*)buf;
    int count = vsprintf(str,fmt,args);

    while(*str) {
        if(*str=='\n')
            uart_send('\r');
        uart_send(*str++);
    }
    __builtin_va_end(args);
    return count;
}

char uart_async_getc() {
    *AUX_MU_IER_REG |= 1;
    lock();
    // Stuck if nothing to read
    while (uart_rx_buffer_isEmpty()) {
        unlock();
        lock();
    }
    char r = uart_rx_buffer[uart_rx_buffer_r_idx++];
    if (uart_rx_buffer_r_idx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_r_idx = 0;
    unlock();
    return r;
}

void uart_async_send(char c) {
    lock();
    // Stuck if buffer is full
    while (uart_tx_buffer_isFull()) {}
    uart_tx_buffer[uart_tx_buffer_w_idx++] = c;
    if (uart_tx_buffer_w_idx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_w_idx = 0;
    unlock();
    *AUX_MU_IER_REG |= 2;           // enable write interrupt
}

int uart_rx_buffer_isEmpty() {
    return uart_rx_buffer_r_idx == uart_rx_buffer_w_idx;
}

int uart_tx_buffer_isEmpty() {
    return uart_tx_buffer_r_idx == uart_tx_buffer_w_idx;
}

int uart_rx_buffer_isFull() {
    return (uart_rx_buffer_w_idx + 1) % VSPRINT_MAX_BUF_SIZE == uart_rx_buffer_r_idx;
}

int uart_tx_buffer_isFull() {
    return (uart_tx_buffer_w_idx + 1) % VSPRINT_MAX_BUF_SIZE == uart_tx_buffer_r_idx;
}

void uart_write_irq_handler() {
    lock();
    // uart_puts("irq write handler\r\n");
    if (uart_tx_buffer_isEmpty()) { // buffer empty
        *AUX_MU_IER_REG &= ~(2);    // disable write interrupt
        unlock();
        return;
    }
    unlock();
    uart_send(uart_tx_buffer[uart_tx_buffer_r_idx]);
    lock();
    uart_tx_buffer_r_idx++;
    if (uart_tx_buffer_r_idx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_r_idx = 0;
    *AUX_MU_IER_REG |= 2;           // enable write interrupt
    unlock();
}

void uart_read_irq_handler() {
    lock();
    // uart_puts("irq read handler\r\n");
    if (uart_rx_buffer_isFull()) {  // buffer full
        *AUX_MU_IER_REG &= ~(1);    // disable read interrupt
        unlock();
        return;
    }
    unlock();
    uart_rx_buffer[uart_rx_buffer_w_idx] = uart_recv();
    lock();
    uart_rx_buffer_w_idx++;
    if (uart_rx_buffer_w_idx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_w_idx = 0;
    *AUX_MU_IER_REG |= 1;
    unlock();
}

void uart_2hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n=(d>>c)&0xF;
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

void uart_flush_FIFO() {
    // https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf Pg.13
    // On write:
    //  Writing with bit 1 set will clear the receive FIFO
    //  Writing with bit 2 set will clear the transmit FIFOF
    *AUX_MU_IIR_REG |= 6;
}

// AUX_MU_IER_REG: BCM2837-ARM-Peripherals.pdf - Pg.12
void uart_interrupt_enable() {
    *AUX_MU_IER_REG |= 1;       // enable read interrupt
    *AUX_MU_IER_REG |= 2;       // enable write interrupt
    *ENABLE_IRQS_1 |= 1 << 29;  // Pg.112
}

void uart_interrupt_disable() {
    *AUX_MU_IER_REG &= ~(1);    // disable read interrupt
    *AUX_MU_IER_REG &= ~(2);    // disable write interrupt
}

void uart_init() {
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLES     |= 1;       // enable UART1
    *AUX_MU_CNTL_REG  = 0;       // disable TX/RX

    /* configure UART */
    *AUX_MU_IER_REG   = 0;       // disable interrupt
    *AUX_MU_LCR_REG   = 3;       // 8 bit data size
    *AUX_MU_MCR_REG   = 0;       // disable flow control
    *AUX_MU_BAUD_REG  = 270;     // 115200 baud rate
    *AUX_MU_IIR_REG   = 6;       // disable FIFO

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~(7<<12);               // clean gpio14
    r |= 2<<12;                  // set gpio14 to alt5
    r &= ~(7<<15);               // clean gpio15
    r |= 2<<15;                  // set gpio15 to alt5
    *GPFSEL1 = r;

    /* enable pin 14, 15 - ref: Page 101 */
    *GPPUD = 0;
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;

    *AUX_MU_CNTL_REG = 3;      // enable TX/RX
}