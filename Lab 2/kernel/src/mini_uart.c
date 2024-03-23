#include "type.h"
#include "gpio.h"
#include "aux.h"
#include "delay.h"


/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void 
mini_uart_init(void)
{
    /*
        setup GPIO pins
    */

    uint32_t selector;

    selector            =   *GPFSEL1;
    selector            &=  ~((7<<12)|(7<<15)); // clean gpio14 and gpio15
    selector            |=  (2<<12)|(2<<15);    // set alt5 for gpio14 and gpio15
    *GPFSEL1            =   selector;

    *GPPUD              =   0;
    
    delay_cycles(150);
    
    *GPPUDCLK0          =   (1<<14)|(1<<15);    // enable pins 14 and 15
    
    delay_cycles(150);
    
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
uint8_t 
mini_uart_getc() 
{
    /* wait until something is in the buffer */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    uint8_t r = (uint8_t)(*AUX_MU_IO & 0xFF);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

uint8_t 
mini_uart_getb() 
{
    /* wait until something is in the buffer */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    return (uint8_t)(*AUX_MU_IO | 0xFF);
}


/**
 * Send a character
 * ref : BCM2837-ARM-Peripherals p5
 * AUX_MU_LSR_REG bit_5 == 1 -> writable
 */
void 
mini_uart_putc(uint32_t c) 
{
    /* wait until we can send */
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}


/**
 * Display a string
 */
void 
mini_uart_puts(byteptr_t s) 
{
    while (*s) {
        mini_uart_putc(*s++);
    }
}

/**
 * Display a string with the newline
 */
void 
mini_uart_putln(byteptr_t s) 
{
    mini_uart_puts(s);
    mini_uart_puts("\r\n");
}


/**
 * Display a 32-bit binary value in hexadecimal
 */
void 
mini_uart_hex(uint32_t d)
{   
    for (int32_t c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        uint8_t n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        mini_uart_putc(n);
    }
}

/**
 * Display a 64-bit binary value in hexadecimal
 */
void 
mini_uart_hexl(uint64_t d)
{
    for (int32_t c = 60; c >= 0; c -= 4) {
        // get highest tetrad
        uint8_t n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        mini_uart_putc(n);
    }
}


void 
mini_uart_endl()
{
    mini_uart_puts("\r\n");
}


void 
mini_uart_space(uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) mini_uart_putc(' '); 
}