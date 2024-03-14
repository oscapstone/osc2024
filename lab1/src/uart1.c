#include "bcm2837/rpi_gpio.h"
#include "bcm2837/rpi_uart1.h"
#include "uart1.h"

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
