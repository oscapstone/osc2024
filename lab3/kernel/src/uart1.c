#include "bcm2837/rpi_gpio.h"
#include "bcm2837/rpi_uart1.h"
#include "bcm2837/rpi_irq.h"
#include "uart1.h"
#include "string.h"



//implement first in first out buffer with a read index and a write index
char uart_tx_buffer[VSPRINT_MAX_BUF_SIZE]={};
unsigned int uart_tx_buffer_widx = 0;  //write index
unsigned int uart_tx_buffer_ridx = 0;  //read index
char uart_rx_buffer[VSPRINT_MAX_BUF_SIZE]={};
unsigned int uart_rx_buffer_widx = 0;
unsigned int uart_rx_buffer_ridx = 0;

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
    *AUX_MU_IIR_REG = 6;    // disable FIFO

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

// AUX_MU_IER_REG -> BCM2837-ARM-Peripherals.pdf - Pg.12
void uart_interrupt_enable(){
    *AUX_MU_IER_REG |= 1;  // enable read interrupt
    *AUX_MU_IER_REG |= 2;  // enable write interrupt
    *ENABLE_IRQS_1  |= 1 << 29;    // Pg.112
}

void uart_interrupt_disable(){
    *AUX_MU_IER_REG &= ~(1);  // disable read interrupt
    *AUX_MU_IER_REG &= ~(2);  // disable write interrupt
}


//scanf
void uart_r_irq_handler()
{
    if ((uart_rx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_rx_buffer_ridx)
    {
        *AUX_MU_IER_REG &= ~(1); // disable read interrupt
        return;
    }
    uart_rx_buffer[uart_rx_buffer_widx++] = uart_recv();
    if (uart_rx_buffer_widx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_widx = 0;
    *AUX_MU_IER_REG |= 1;
}

//printf
void uart_w_irq_handler()
{
    if (uart_tx_buffer_ridx == uart_tx_buffer_widx)
    {
        *AUX_MU_IER_REG &= ~(2); // disable write interrupt
        return;                  // buffer empty
    }
    uart_send(uart_tx_buffer[uart_tx_buffer_ridx++]);
    if (uart_tx_buffer_ridx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_ridx = 0;
    *AUX_MU_IER_REG |= 2; // enable write interrupt
}

// uart_async_getc read from buffer
// uart_r_irq_handler write to buffer then output
char uart_async_recv()
{
    *AUX_MU_IER_REG |= 1; // enable read interrupt
    // do while if buffer empty
    while (uart_rx_buffer_ridx == uart_rx_buffer_widx)
        *AUX_MU_IER_REG |= 1; // enable read interrupt
    el1_interrupt_disable();
    char r = uart_rx_buffer[uart_rx_buffer_ridx++];
    if (uart_rx_buffer_ridx >= VSPRINT_MAX_BUF_SIZE)
        uart_rx_buffer_ridx = 0;
    el1_interrupt_enable();
    return r;
}


// uart_async_putc writes to buffer
// uart_w_irq_handler read from buffer then output
void uart_async_send(char c)
{
    // if buffer full, wait for uart_w_irq_handler
    while ((uart_tx_buffer_widx + 1) % VSPRINT_MAX_BUF_SIZE == uart_tx_buffer_ridx)
        *AUX_MU_IER_REG |= 2; // enable write interrupt
    el1_interrupt_disable();
    uart_tx_buffer[uart_tx_buffer_widx++] = c;
    if (uart_tx_buffer_widx >= VSPRINT_MAX_BUF_SIZE)
        uart_tx_buffer_widx = 0; // cycle pointer
    el1_interrupt_enable();
    *AUX_MU_IER_REG |= 2; // enable write interrupt
}

int  uart_sendlinek(char* fmt, ...) {
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