
#include "uart.h"

void uart_init() {
	
	register unsigned int r;
	
	// according to BCM2835 ARM peripherals pg. 101 to setup the GPIO pull-up/down clock registers
	// disable pull-up and pull-down
	*GPPUD = 0;
	r = 150; while (r--) { asm volatile("nop"); }
	*GPPUDCLK0 = (1 << 14)|(1 << 15);
	r = 150; while (r--) { asm volatile("nop"); }
	*GPPUDCLK0 = 0;
	
	// 
	r = 500; while (r--) { asm volatile("nop"); }	
	
	// 1. set AUXENB register to enable mini UART.
	*AUX_ENABLE |= 1;
	// 2. Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
	*AUX_MU_CNTL = 0;
	// 3. Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
	*AUX_MU_IER = 0;
	// 4. Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
	*AUX_MU_LCR = 3;
	// 5. Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
	*AUX_MU_MCR = 0;
	// 6. Set AUX_MU_BAUD to 270. Set baud rate to 115200
	// by calculation the BAUD reg should be 270.2673611111 = 270
	*AUX_MU_BAUD = 270;
	// 7. Set AUX_MU_IIR_REG to 6. No FIFO.
	// 31:8 Reserved, 7:6 FIFO enables, 5:4 zero, 2:1 READ bits WRITE bits
	//	    76543210
	//  0x6 00000110
	*AUX_MU_IIR = 0x6;
	// 8. Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
	*AUX_MU_CNTL = 3;

	r = *GPFSEL1;
	r &= ~((7<<12)|(7<<15)); // gpio14, gpio15 clear to 0
	r |= (2<<12)|(2<<15);    // set gpio14 and 15 to 010/010 which is alt5
	*GPFSEL1 = r;          // from here activate Trasmitter&Receiver
}

/**
 * Send a character
 */
void uart_send_char(unsigned int c) {
    /* wait until we can send */
    // P.15 AUX_MU_LSR register shows the data(line) status
    // AUX_MU_LSR bit 5 => 0x20 = 00100000
    // bit 5 is set if the transmit FIFO can accept at least one byte.  
    // &0x20 can preserve 5th bit, if bit 5 set 1 can get !true = false leave loop
    // else FIFO can not accept at lease one byte then still wait 
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    //P.11 The AUX_MU_IO_REG register is primary used to write data to and read data from the
    //UART FIFOs.
    //communicate with(send to) the minicom and print to the screen 
    *AUX_MU_IO=c;
}

/**
 * Receive a character
 */
char uart_get_char() {
    char r;
    /* wait until something is in the buffer */
    //bit 0 is set if the receive FIFO holds at least 1 symbol.
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

/**
 * Display a string
 */
void uart_send_string(char* s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send_char('\r');
        uart_send_char(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send_char(n);
    }
}
