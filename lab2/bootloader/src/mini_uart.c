#include "rpi_mini_uart.h"
#include "rpi_gpio.h"

void uart_init ( void )
{
    register int selector;

    *AUX_ENABLES     |= 1;                   //Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG  = 0;                  //Disable auto flow control and disable receiver and transmitter (for now)
    
    selector = *(GPFSEL1);
    selector &= ~(7<<12);                   // clean gpio14
    selector |= 2<<12;                      // set alt5 for gpio14
    selector &= ~(7<<15);                   // clean gpio15
    selector |= 2<<15;                      // set alt5 for gpio 15
    *GPFSEL1 = selector;

    *GPPUD = 0;
    selector=150; while(selector--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    selector=150; while(selector--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;

    
    *AUX_MU_IER_REG = 0;                //Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3;                //Enable 8 bit mode
    *AUX_MU_MCR_REG = 0;                //Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270;             //Set baud rate to 115200

    *AUX_MU_CNTL_REG = 3;              //Finally, enable transmitter and receiver
}

char uart_recv() {
    char r;
    while(!(*AUX_MU_LSR_REG & 0x01)){};
    r = (char)(*AUX_MU_IO_REG);
    return r=='\r'?'\n':r;
}

void uart_send(unsigned int c) {
    while(!(*AUX_MU_LSR_REG & 0x20)){};
    *AUX_MU_IO_REG = c;
}

void uart_puts(char *str) {
    while(*str) {
        if(*str=='\n')
            uart_send('\r');
        uart_send(*str++);
    }
}

// Function to print an integer to the UART
void put_int(int num)
{
    // Handle the case when the number is 0
    if (num == 0)
    {
        uart_send('0');
        return;
    }

    // Temporary array to store the reversed digits as characters
    char temp[12]; // Assuming int can have at most 10 digits
    int idx = 0;

    // Handle negative numbers
    if (num < 0)
    {
        uart_send('-');
        num = -num;
    }

    // Convert the number to characters and store in the temporary array in reverse order
    while (num > 0)
    {
        temp[idx++] = (char)(num % 10 + '0');
        num /= 10;
    }

    // Reverse output the character digits
    while (idx > 0)
    {
        uart_send(temp[--idx]);
    }
}
char uart_get(){
    char r;
    while(!(*AUX_MU_LSR_REG & 0x01)){};
    r = (char)(*AUX_MU_IO_REG);
    return r;
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