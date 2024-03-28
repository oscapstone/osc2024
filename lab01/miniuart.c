// miniuart.c
#include "miniuart.h"
#include "gpio.h"


void miniUARTInit() {
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1;   // Enable mini UART
    *AUX_MU_CNTL = 0;    // Disable TX, RX during configuration
    *AUX_MU_IER = 0;     // Disable interrupt
    *AUX_MU_LCR = 3;     // Set the data size to 8 bit
    *AUX_MU_MCR = 0;     // Don't need auto flow control
    *AUX_MU_BAUD = 270;  // Set baud rate to 115200
    *AUX_MU_IIR = 6;     // No FIFO

    /* map UART1 to GPIO pins */
    // page 101
    r =* GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15 innitial ->p92
    r |= (2 << 12) | (2 << 15);    // alt5 ->p102
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx // bit1:transmitter bit0:receiver
}

char miniUARTRead(void) {
    char r;
    /* wait until something is in the buffer */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR & 0x01)); // This bit is set if the receive FIFO holds at least 1 symbol. 
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carrige return to newline */
    return r == '\r'?'\n':r;
}

void miniUARTWrite(char c) {
    /* wait until we can send */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR & 0x20)); // This bit is set if the transmit FIFO can accept at least one byte. 
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

void miniUARTWriteString(const char* s) {
    while(*s) {
        /* convert newline to carrige return + newline */
        if(*s == '\n')
            miniUARTWrite('\r');
        miniUARTWrite(*s++);
    }
}

void miniUARTWriteHex(unsigned int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        miniUARTWrite(n);
    }
}
