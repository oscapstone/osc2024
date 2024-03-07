#include "gpio.h"
#include "delays.h"

/* Auxilary mini UART registers */
#define AUX_ENABLES         ((volatile unsigned int*)(MMIO_BASE+0x00215004))    // Auxiliary enables    | 3
#define AUX_MU_IO_REG       ((volatile unsigned int*)(MMIO_BASE+0x00215040))    // I/O Data             | 8
#define AUX_MU_IER_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215044))    // Interrupt Enable     | 8
#define AUX_MU_IIR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215048))    // Interrupt Identify   | 8
#define AUX_MU_LCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))    // Line Control         | 8
#define AUX_MU_MCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215050))    // Modem Control        | 8
#define AUX_MU_LSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215054))    // Line Status          | 8
#define AUX_MU_MSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215058))    // Modem Status         | 8
#define AUX_MU_SCRATCH      ((volatile unsigned int*)(MMIO_BASE+0x0021505C))    // Scratch              | 8
#define AUX_MU_CNTL_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215060))    // Extra Control        | 8
#define AUX_MU_STAT_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215064))    // Extra Status         | 32
#define AUX_MU_BAUD_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215068))    // Baudrate             | 16



/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void mini_uart_init (void)
{
    unsigned int selector;

    selector            =   *GPFSEL1;
    selector            &=  ~((7<<12)|(7<<15));         // clean gpio14 and gpio15
    selector            |=  (2<<12) | (2<<15);          // set alt5 for gpio14 and gpio15
    *GPFSEL1            =   selector;

    *GPPUD              =   0;
    
    wait_cycles(150);
    
    *GPPUDCLK0          =   (1<<14)|(1<<15);    // enable pins 14 and 15
    
    wait_cycles(150);
    
    *GPPUDCLK0          =   0;                  // flush GPIO setup


    *AUX_ENABLES        |=  1;                  // Enable UART1, AUX mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG    =   0;                  // Disable auto flow control and disable receiver and transmitter (for now)

    *AUX_MU_LCR_REG     =   3;                  // Enable 8 bit mode
    *AUX_MU_MCR_REG     =   0;                  // Set RTS line to be always high

    *AUX_MU_IER_REG     =   0;                  // Disable receive and transmit interrupts

    *AUX_MU_IIR_REG     =   0xc6;               // disable interrupts
    *AUX_MU_BAUD_REG    =   270;                // Set baud rate to 115200

    *AUX_MU_CNTL_REG    =   3;                  // Finally, enable transmitter and receiver
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
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR_REG & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO_REG);
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
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR_REG & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO_REG = c;
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
