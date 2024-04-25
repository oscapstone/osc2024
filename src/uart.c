#include "uart.h"

void uart_init()
{
    // initialize auxiliary registers
    *UART_AUXENB = 1;
    *UART_AUX_MU_CNTL_REG = 0;
    *UART_AUX_MU_IER_REG = 0;   // disable interrupt
    *UART_AUX_MU_LCR_REG = 3;
    *UART_AUX_MU_MCR_REG = 0;
    *UART_AUX_MU_BAUD = 270;    // baud rate would be 115200
    *UART_AUX_MU_IIR_REG = 6;   // not 0xc6, because FIFO disabled

    // enable uart on GPIO
    unsigned int r;     // TODO: is keyword "register" required?
    r = *UART_GPFSEL1;
    r &= ~((7<<15)|(7<<12));
    r |= ((2<<15)|(2<<12));
    *UART_GPFSEL1 = r;

    *UART_GPPUD = 0;
    // wait 150 cycles for GPPUD
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *UART_GPPUDCLK0 = ((1<<15)|(1<<14));    // disable pull up/down on PIN 14/15

    // wait 150 cycles for GPPUD
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *UART_GPPUDCLK0 = 0;

    *UART_AUX_MU_CNTL_REG = 3;  // enable Tx/Rx
}

void uart_init_buffer(void)
{
    rb_init(&tx_buf, RBUFFER_SIZE);
    rb_init(&rx_buf, RBUFFER_SIZE);
}

void uart_send(unsigned int c) {
    while (!((*UART_AUX_MU_LSR_REG) & 32)) {
        asm volatile("nop");
    }

    *UART_AUX_MU_IO_REG = c;
}

void uart_puts(char* s) {
    while (*s) {
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

char uart_getc() {
    // check status in LSR
    while (!((*UART_AUX_MU_LSR_REG) & 1)) {
        asm volatile("nop");
    }

    // read data from IO
    char c;
    c = (char) (*UART_AUX_MU_IO_REG);
    return c == '\r' ? '\n' : c;
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

void uart_putints(int d)
{
    char buffer[100];
    int idx = 0;
    if (d == 0) {
        buffer[idx++] = '0';
    } else {
        while (d) {
            buffer[idx++] = '0' + (d % 10);
            d /= 10;
        }
    }
    while (idx > 0) {
        uart_send(buffer[--idx]);
    }
}

void uart_putuints(unsigned int d)
{
    char buffer[100];
    int idx = 0;
    if (d == 0) {
        buffer[idx++] = '0';
    } else {
        while (d) {
            buffer[idx++] = '0' + (d % 10);
            d /= 10;
        }
    }
    while (idx > 0) {
        uart_send(buffer[--idx]);
    }
}

void uart_putlong(long d)
{
    char buffer[100];
    int idx = 0;
    if (d == 0) {
        buffer[idx++] = '0';
    } else {
        while (d) {
            buffer[idx++] = '0' + (d % 10);
            d /= 10;
        }
    }
    while (idx > 0) {
        uart_send(buffer[--idx]);
    }
}

void uart_async_send(unsigned int c)
{
    if (!rb_full(&tx_buf)) {
        rb_write(&tx_buf, c);
    }
    uart_enable_tx_interrupt();
}

void uart_async_puts(char *s)
{
    while (*s) {
        if(*s == '\n') {
            uart_async_send('\r');
        }
        uart_async_send(*s++);
    }
    uart_enable_tx_interrupt();
}

char uart_async_getc()
{
    uart_enable_rx_interrupt();
    while (rb_empty(&rx_buf)) {}

    return rb_read(&rx_buf);
}

void uart_tx_handler(void)
{
    while (!rb_empty(&tx_buf)) {
        uart_send(rb_read(&tx_buf));
    }
}

void uart_rx_handler(void)
{
    char c;
    c = uart_getc();

    if (!rb_full(&rx_buf)) {
        rb_write(&rx_buf, c);
    }
}

void uart_disable_rx_interrupt(void)
{
    *UART_AUX_MU_IER_REG &= ~(1);
}

void uart_disable_tx_interrupt(void)
{
    *UART_AUX_MU_IER_REG &= ~(2);
}

void uart_enable_rx_interrupt(void)
{
    *UART_AUX_MU_IER_REG |= (1);
}

void uart_enable_tx_interrupt(void)
{
    *UART_AUX_MU_IER_REG |= (2);
}
