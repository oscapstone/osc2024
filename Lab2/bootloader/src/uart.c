#include "uart.h"

unsigned int testpoint;

void uart_init(){
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1;   //Enable mini uart
    *AUX_MU_CNTL_REG = 0;   //Disable auto flow control, reciver, and transmitter
    *AUX_MU_IER_REG = 0;    //Disable recive and transmit interrupts
    *AUX_MU_LCR_REG = 3;    //Enalbe 8 bit mode
    *AUX_MU_MCR_REG = 0;    //Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270; //Set baud rate to 115200
    *AUX_MU_IIR_REG = 0xc6; //0xc6 = 11000110 
                        //bit 6 bit 7 No FIFO. Sacrifice reliability(buffer) to get low latency 
                        //Writing with bit 1 set will clear the receive FIFO
                        //Writing with bit 2 set will clear the transmit FIFO
    *AUX_MU_CNTL_REG = 3;   //enable transmitter and receiver

    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15 clear to 0
    r|=(2<<12)|(2<<15);    // set gpio14 and 15 to 010/010 which is alt5
    *GPFSEL1 = r;          // from here activate Trasmitter&Receiver

    *GPPUD = 0;
    r=150; while(r--){ asm volatile("nop"); } //delay(150)
    *GPPUDCLK0 = (1<<14)|(1<<15);

    r=150; while(r--){ asm volatile("nop"); } //delay(150)
    *GPPUDCLK0 = 0;        // flush GPIO setup
    r=500; while(r--){ asm volatile("nop"); } //delay(500)

}

void uart_send_char(unsigned int c){
    while(!(*AUX_MU_LSR_REG&0x20));
    *AUX_MU_IO_REG=c;
}

char uart_get_char(){
    char r;
    while(!(*AUX_MU_LSR_REG&0x01));
    r=(char)(*AUX_MU_IO_REG);
    return r=='\r'?'\n':r;
}

void uart_display_string(char* s){
    while(*s) {
        if(*s=='\n'){
            uart_send_char('\r');
        }        
        uart_send_char(*s++);
    }
}

void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_display_string("0x");
    for(c=28;c>=0;c-=4) {
        // get highest tetrad(4 bits)
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n>9 ? 0x37 : 0x30;
        uart_send_char(n);
    }
}

char uart_get_raw(){
    /*
    bit_0 == 1 -> readable
    0x01 = 0000 0000 0000 0001
    ref BCM2837-ARM-Peripherals p5
    */
    while (!(*(AUX_MU_LSR_REG)&0x01))
    {
    }
    char temp = *(AUX_MU_IO_REG)&0xFF;
    return temp;
}