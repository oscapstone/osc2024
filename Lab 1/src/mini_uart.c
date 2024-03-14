#include "gpio.h"
#include "aux.h"
#include "delays.h"


/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void mini_uart_init(void)
{
    /*

        setup GPIO pins

    */

    unsigned int selector;

    selector            =   *GPFSEL1;
    selector            &=  ~((7<<12)|(7<<15)); // clean gpio14 and gpio15
    selector            |=  (2<<12)|(2<<15);    // set alt5 for gpio14 and gpio15
    *GPFSEL1            =   selector;

    *GPPUD              =   0;
    
    wait_cycles(150);
    
    *GPPUDCLK0          =   (1<<14)|(1<<15);    // enable pins 14 and 15
    
    wait_cycles(150);
    
    *GPPUDCLK0          =   0;                  // flush GPIO setup

    /* 
    
        setup AUX mini UART 
    
    */

    *AUX_ENABLES        |=  1;                  // Enable UART1, AUX mini uart (this also enables access to its registers)
    *AUX_MU_CNTL        =   0;                  // Disable auto flow control and disable receiver and transmitter (for now)

    *AUX_MU_LCR         =   3;                  // Enable 8 bit mode
    *AUX_MU_MCR         =   0;                  // Set RTS line to be always high

    *AUX_MU_IER         =   0;                  // Disable receive and transmit interrupts

    *AUX_MU_IIR         =   0xc6;               // disable interrupts
    *AUX_MU_BAUD        =   270;                // Set baud rate to 115200

    *AUX_MU_CNTL        =   3;                  // Finally, enable transmitter and receiver
}


/**
 * Receive a character
 * ref : BCM2837-ARM-Peripherals p5
 * AUX_MU_LSR_REG bit_0 == 1 -> readable
 */
char mini_uart_getc() 
{
    char r;
    /* wait until something is in the buffer */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}


/**
 * Send a character
 * ref : BCM2837-ARM-Peripherals p5
 * AUX_MU_LSR_REG bit_5 == 1 -> writable
 */
void mini_uart_putc(unsigned int c) 
{
    /* wait until we can send */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}


/**
 * Display a string
 */
void mini_uart_puts(char *s) 
{
    while (*s) {
        mini_uart_putc(*s++);
    }
}

/**
 * Display a string with the newline
 */
void mini_uart_putln(char *s) 
{
    mini_uart_puts(s);
    mini_uart_puts("\r\n");
}


/**
 * Display a binary value in hexadecimal
 */
void mini_uart_hex(unsigned int d)
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        mini_uart_putc(n);
    }
}
