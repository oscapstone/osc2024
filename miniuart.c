#include "headers/uart.h"

void init(void)
{
    // Since GPIO is set to alternate function, we need to disable the pull up/down.
    *GPPUD = 0;
    unsigned int r = 150; while(r--){ asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14) | (1<<15);
    r = 150; while(r--){ asm volatile("nop"); }
    *GPPUDCLK0 = 0;
    r = 500; while(r--){asm volatile("nop"); }

    *AUX_ENABLES |= 1;           // enable Mini UART
    *AUX_MU_CNTL_REG = 0;       // tx and rx
    *AUX_MU_IER_REG = 0;        // interrupt enable register
    *AUX_MU_LCR_REG = 3;        // data format and DLAB access
    *AUX_MU_MCR_REG = 0;        // control the 'modem' signals
    *AUX_MU_BAUD_REG = 270;     // baud rate
    *AUX_MU_IIR_REG = 0xc6;        // interrupt status
    *AUX_MU_CNTL_REG = 3;

    // p.92 describes how to set alternate functions for GPIO
    // for rpi: GPIO14:TX, GPIO15:RX, mini UART should set ALT5(010)
    unsigned int temp_reg = *GPFSEL1;   
    temp_reg &= ~(7<<12 | 7<<15);
    temp_reg |= (2<<12 | 2<<15);
    *GPFSEL1 = temp_reg;
}

char receive(void)
{
    char r;
    do{asm volatile("nop");}while(!(*AUX_MU_LSR_REG&0x01));
    r = (char)(*AUX_MU_IO_REG);
    return r=='\r'?'\n':r;
}

void send(char r)
{
    unsigned int t = r;
    do{asm volatile("nop");}while(!(*AUX_MU_LSR_REG&0x20));
    *AUX_MU_IO_REG = t;
}

void display(char *string)
{
    while(!(*string == '\0'))
    {
        if(*string=='\n') send('\r');
        send(*string++);
    }
}