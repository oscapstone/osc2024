#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

#include "mini_uart.h"
#include "string.h"

void uart_init()
{
    // set the alternate function for GPIO14 and GPIO15
    unsigned int selector;
    selector = *GPFSEL1;
    selector &= ~(7<<12); // size gpio14 to 0 for gpio14 is bits 12-14 
    selector |= 2<<12; // set to alt5 for gpio14
    selector &= ~(7<<15); // size gpio15 to 0 for gpio15 is bits 15-17
    selector |= 2<<15; // set to alt5 for gpio15
    *GPFSEL1 = selector;

    // GPIO initilaization
    *GPPUD = 0; // disable pull up/down for all GPIO pins
    unsigned int i = 150;
    while(i--) asm volatile("nop");
    *GPPUDCLK0 = (1<<14) | (1<<15); // enable clock for gpio14 and gpio15
    i = 150;
    while(i--) asm volatile("nop");
    *GPPUDCLK0 = 0; // disable clock for gpio14 and gpio15

    // initialize mini uart
    *AUX_ENABLES = 1;        //Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG = 0;   //Disable auto flow control and disable receiver and transmitter (for now)
    *AUX_MU_IER_REG = 0;    //Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3;    //Enable 8 bit mode
    *AUX_MU_MCR_REG = 0;    //Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270; // 115200 baud
    *AUX_MU_CNTL_REG = 3;   //Finally, enable transmitter and receiver
}

char uart_recv()
{

    char r;
    // bit 0 == 1, if receiver holds valid byte
    do{asm volatile("nop");}while(!(*AUX_MU_LSR_REG & 0x01));
    r = (char)(*AUX_MU_IO_REG);
    return r == '\r'?'\n':r;
    
}

void uart_send(const char c)
{
    // bit 6 == 1, if transmitter is empty
    while (1) {
        if ((*AUX_MU_LSR_REG)&0x20) break;
    }
    *AUX_MU_IO_REG = c;

}

void uart_send_string(const char* str)
{
    while(*str){
        if(*str == '\n')
            uart_send('\r');
        uart_send(*str++);
    }
}