#include "gpio.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;
    
    r = *GPFSEL1;
    //gpio15 can be both used for mini UART and PL011 UART
    r &= ~((7<<12)|(7<<15)); // gpio14 least bit is 12, gpio15 least bit is 15
    r |= (2<<12)|(2<<15);    // set alt5 for gpio14 and gpio15
    *GPFSEL1 = r;          // control gpio pin 10~19
    *GPPUD = 0;            // enable pins 14 and 15
    // gpio pull up/down 
    // show hackmd notes to TA
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    
    /* initialize UART */
    *AUX_ENABLE |= 1;      // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;      // disable tx,rx during configuration
    *AUX_MU_IER = 0;       // disable tx/rx interrupts
    *AUX_MU_LCR = 3;       // set data size 8 bits
    *AUX_MU_MCR = 0;       // don't need auto flow control
    *AUX_MU_BAUD = 270;    // 115200 baud, system clock 250MHz
    *AUX_MU_IIR = 0x6;     // clear FIFO
    *AUX_MU_CNTL = 3;      // enable Tx, Rx
    
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}


char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x01));

    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}


void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
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