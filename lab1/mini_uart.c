#include "headers/mini_uart.h"

void delay( register unsigned int time)
{
    while ( time == 0)
    {
        asm volatile("nop");
        time -= 1;
    }// while
    return;
}

void mini_uart_init()
{
    // connecting to GPIO pins 14 & 15
    // only using 3 bits
    register unsigned int reg = *GPFSEL0;
    reg &= ~(( 0xF >> 1) << 12); // clean them
    reg &= ~(( 0xF >> 1) << 15);
    reg |= ( 0x2 << 12); // set them
    reg |= ( 0x2 << 15);
    *GPFSEL0 = reg; // write back

    // commit the signal
    // need to disable GPIO pull-up/down functions
    *GPPUD = 0;
    delay( 150);
    *GPPUDCLK0 = ( 1 << 14) | ( 1 << 15); // select the pins
    delay( 150);
    *GPPUDCLK0 = 0; // reset it

    // set uart enable
    *AUX_ENABLES |= 1;

    *AUX_MU_CNTL_REG = 0;
    *AUX_MU_IER_REG = 0;
    *AUX_MU_LCR_REG = 3;
    *AUX_MU_MCR_REG = 0;
    *AUX_MU_BAUD_REG = 270;
    *AUX_MU_IIR_REG = 6;
    *AUX_MU_CNTL_REG = 3;

    return;
}

char mini_uart_getc()
{
    // check the status of uart
    while ( 1)
    {
        if ( *AUX_MU_LSR_REG & 0x1)
        {
            break;
        }// if
    }// while

    char recv = (char)*AUX_MU_IO_REG;

    // convert the carrige return
    return recv == '\r' ? '\n': recv;
}

void mini_uart_putc( char c)
{
    // check the status of uart
    while ( 1)
    {
        if ( *AUX_MU_LSR_REG & ( 0x1 << 5))
        {
            break;
        }// if
    }// while

    *AUX_MU_IO_REG = (unsigned int)c;
    return;
}

void mini_uart_puts( char *s)
{
    while ( *s != '\0')
    {
        mini_uart_putc( *s);
        s += 1;
    }// while
    
    return;
}
