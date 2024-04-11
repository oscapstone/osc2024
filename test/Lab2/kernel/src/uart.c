#include "gpio.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))
#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long addr, unsigned int value)
{
    volatile unsigned int *point = (unsigned int *)addr;
    *point = value;
}

void reset(int tick)
{                                     // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20); // full reset
    set(PM_WDOG, PM_PASSWORD | tick); // number of watchdog tick
}

void cancel_reset()
{
    set(PM_RSTC, PM_PASSWORD | 0); // full reset
    set(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
}

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1; // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3; // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6; // disable interrupts
    *AUX_MU_BAUD = 270; // 115200 baud
    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0; // enable pins 14 and 15
    r = 150;
    while (r--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;   // flush GPIO setup
    *AUX_MU_CNTL = 3; // enable Tx, Rx
}

/**
 * Send a character
 */
void uart_send(unsigned int c)
{
    /* wait until we can send */
    do
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

/**
 * Receive a character
 */
char uart_getc()
{
    char r;
    /* wait until something is in the buffer */
    do
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

/**
 * Display a string
 */
void uart_puts(char *s)
{
    while (*s)
    {
        /* convert newline to carriage return + newline */
        if (*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

void uart_getcommand(char *command)
{
    int idx = 0;
    command[idx] = uart_getc();
    uart_send(command[idx]);
    while (command[idx] != '\n')
    {
        command[++idx] = uart_getc();
        uart_send(command[idx]);
    }
    command[idx] = '\0';
    uart_send('\r');
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_send('0');
    uart_send('x');
    for(c=28;c>=0;c-=4) { //28 24 20 16 12 8 4 0
        // get highest tetrad
        n=(d>>c)&0xF;   //get 3:0 bit
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        //n+=n>9?0x37:0x30;
        if(n > 9){
            n += 0x37;
        }
        else{
            n += 0x30;
        }
        uart_send(n);
    }
}

/**
 * Display a binary value in hexadecimal without 0x
 */
void uart_hex_no0x(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) { //28 24 20 16 12 8 4 0
        // get highest tetrad
        n=(d>>c)&0xF;   //get 3:0 bit
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        //n+=n>9?0x37:0x30;
        if(n > 9){
            n += 0x37;
        }
        else{
            n += 0x30;
        }
        uart_send(n);
    }
}

void uart_uint(unsigned int i)
{
    char c[10];
    if (i == 0)
    {
        uart_send('0');
        return;
    }
    int digits = -1;
    while (i != 0)
    {
        c[++digits] = '0' + i % 10;
        i /= 10;
    }
    for (; digits >= 0; --digits)
    {
        uart_send(c[digits]);
    }
}