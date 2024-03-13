#include "uart.h"
// refer to https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf


void uart_init(){
    register unsigned int r;
    //Since We've set alt5, we want to disable basic input/output
    //To achieve this, we need diable pull-up and pull-dwon
    *GPPUD = 0;   //  P101 top. 00- = off - disable pull-up/down 
    //Wait 150 cycles
    //this provides the required set-up time for the control signal 
    r=150; while(r--) { asm volatile("nop"); }
    // GPIO control 54 pins
    // GPPUDCLK0 controls 0-31 pins
    // GPPUDCLK1 controls 32-53 pins
    // set 14,15 bits = 1 which means we will modify these two bits
    // trigger: set pins to 1 and wait for one clock
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup

    r=500; while(r--) { asm volatile("nop"); }
    
    //-------------initialize uart-------------------
    //Set AUXENB register to enable mini UART.
    *AUX_ENABLE |=1;
    //    0x????????
    //or  0x00000001
    //---------------
    //    0x???????1  -> set 0 bits to 1
    
    //Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    *AUX_MU_CNTL =0;
    
    //Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_IER =0;
    
    //Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    *AUX_MU_LCR =3;
    
    //Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    *AUX_MU_MCR =0;
    
    //Set AUX_MU_BAUD to 270. Set baud rate to 115200
    *AUX_MU_BAUD =270;
    
    //Set AUX_MU_IIR_REG to 6. No FIFO.
    *AUX_MU_IIR =0xc6;
    
    //Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
    *AUX_MU_CNTL =3;
    //----------------------------------------
    
    // map UART1 to GPIO pins, GPFSEL1 controls 10-19
    r=*GPFSEL1;
    
    // gpio14(TXD), gpio15(RXD) clear to 0
    r&=~((7<<12)|(7<<15));
    // 000000????????????(12~17bits set to 0)
    
    // set gpio14 and gpio15 to 010/010 which is ALT15
    r|=(2<<12)|(2<<15);
    // 010010000000000000
    
    // activate Trasmitter&Receiver
    *GPFSEL1 = r;
}


void uart_send_char(unsigned int c){
    /* wait until we can send */
    // AUX_MU_LSR bit 5 => 0x20 = 00100000
    // bit 5 is set if the transmit FIFO can accept at least one byte.  
    // &0x20 can preserve 5th bit, if bit 5 set 1 can get !true = false leave loop
    // else FIFO can not accept at lease one byte then still wait 
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    //P.11 The AUX_MU_IO_REG register is primary used to write data to and read data from the UART FIFOs.
    //communicate with(send to) the minicom and print to the screen 
    *AUX_MU_IO=c;
}


char uart_get_char(){
    char r;
    /* wait until something is in the buffer */
    //bit 0 is set if the receive FIFO holds at least 1 symbol.
    //0x01 & 0x01 = 0x01
    //~(0x01)=0x10, leave while loop to get char
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    // convert char to newline
    if(r == '\r') return '\n';
    else return r;
}


void uart_send_string(char *s){
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send_char('\r');
        uart_send_char(*s++);
    }
}

// convert binary to hex and print char
void bin_to_hex(unsigned int b){
    unsigned int n;
    int c;
    uart_send_string("0x");
    for(c=28;c>=0;c-=4) {
        // get highest tetrad(4 bits), total 8 bits(ex:0x73A2E6C)
        n=(b>>c)&0xF;
        // 0111(7), 0111>>28=0111, 0111 & 1111=0111
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += (n>9 ? 0x37 : 0x30);
        // 1001(A>9), 10(A) + 0x37 = 0x41
        uart_send_char(n);
    }
}
