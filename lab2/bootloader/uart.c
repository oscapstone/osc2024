#include"header/uart.h"

void uart_init(){

    *AUX_ENABLE |= 1; 
    *AUX_MU_CNTL = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_LCR = 3;       
    *AUX_MU_MCR = 0;
    *AUX_MU_BAUD = 270;

    register unsigned int r;

    //???
    r =* GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15 innitial
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    
    *AUX_MU_IIR = 6;
    *AUX_MU_CNTL = 3;
}

void uart_send_char(unsigned int c){
    do{asm volatile("nop");}while(!(*AUX_MU_LSR & 0x20)); // This bit is set if the transmit FIFO can accept at least one byte. 
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

char uart_get_char(){
    char r;
    /* wait until something is in the buffer */
    //bit 0 is set if the receive FIFO holds at least 1 symbol.
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}
char uart_get_img_char(){
    char r;
    /* wait until something is in the buffer */
    //bit 0 is set if the receive FIFO holds at least 1 symbol.
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    return r;
}
void uart_send_str(char *s){
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send_char('\r');
        uart_send_char(*s++);
    }
}

void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_send_str("0x");
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send_char(n);
    }
}