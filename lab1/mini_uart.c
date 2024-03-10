#include "headers/mini_uart.h"

void mini_uart_init()
{
    register unsigned int reg;

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

}

void mini_uart_putc( char c);
void mini_uart_puts( char *s);
