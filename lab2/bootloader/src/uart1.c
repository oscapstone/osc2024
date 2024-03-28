#include "bcm2837/rpi_gpio.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"

void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLES      = 1;       // enable UART1
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

char uart_recv_loading() {
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

void uart_send(unsigned int c) {
    while(!(*AUX_MU_LSR_REG & 0x20)){};
    *AUX_MU_IO_REG = c;
}

void uart_puts(char *str) {
    while(*str) {
        if(*str=='\n')
            uart_send('\r');
        uart_send(*str++);
    }
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
