#include "gpio.h"

/* Auxilary mini UART registers */
#define AUXENB              ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO_REG       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH_REG  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD         ((volatile unsigned int*)(MMIO_BASE+0x00215068))

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART1 */
    *AUXENB |=1;            // enable UART1, AUX mini uart 
    *AUX_MU_CNTL_REG = 0;   // Disable transmitter and receiver (Tx, Rx)
    *AUX_MU_IER_REG = 0;    // Disable interrupt
    *AUX_MU_LCR_REG = 3;    // data size to 8 bit
    *AUX_MU_MCR_REG = 0;    // Don’t need auto flow control.
    *AUX_MU_IIR_REG = 0xc6; // disable interrupts and clear FIFO
    *AUX_MU_BAUD = 270;     // 115200 baud rate
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15));  // gpio14, gpio15 init to zero
    r|=(2<<12)|(2<<15);     // set alt5 (UART1's Tx, Rx)
    *GPFSEL1 = r;           // alternate function
    *GPPUD = 0;             // enable gpio14, gpio15
    r=150; while(r--) { asm volatile("nop"); }  // wait 150 cycles
    *GPPUDCLK0 = (1<<14)|(1<<15);   // gpio14, gpio15 is gppud
    r=150; while(r--) { asm volatile("nop"); }  // wait 150 cycles
    *GPPUDCLK0 = 0;         // flush GPIO setup avoid other GPIO interference
    *AUX_MU_CNTL_REG = 3;   // enable transmitter and receiver (Tx, Rx)
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    // check Line Status Register (LSR) Transmit Holding Register Empty 5 
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        asm volatile("nop");
    }
    // 
    *AUX_MU_IO_REG = c;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    // check Line Status Register (LSR) Rx Buffer Data Ready 0
    while (!(*AUX_MU_LSR_REG & 0x01)) {
        asm volatile("nop");
    }
    /* read it and return */
    r=(char)(*AUX_MU_IO_REG);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

/**
 * Display a string
 */
void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // Shift 'd' right by 'c' bits to align the current nibble at the right-most position and mask out the rest
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'a'-'f'
        n+=n>9?0x57:0x30;
        uart_send(n);
    }
}