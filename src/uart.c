/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "gpio.h"
#include "mbox.h"
#include "delays.h"
#include "uart.h"

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1;      // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;      // Disable transmitter and receiver during configuration.
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;       // RTS line high
    *AUX_MU_IER = 0;       // Disable interrupts.
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r = 150;
    while (r--)
        asm volatile("nop");
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--)
        asm volatile("nop");
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx
}

void uart_async_init()
{

    *AUX_MU_IER = 0x1; // enable receive interrupt. Send interrupt when receive FIFO is not empty
    *IRQ_ENABLE1 |= (1 << 29); // set IRQ_ENABLE to enable mini UART receive interrupt (at pending bit 29)
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    r = (char) (*AUX_MU_IO);
    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

/**
 * Display a string
 */
void uart_puts(char *s)
{
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d)
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

int check_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}


/**
 * Display a string
 */
void printf(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    // we don't have memory allocation yet, so we
    // simply place our string after our code
    char *s = (char*)&_end;
    // use sprintf to format our string
    vsprintf(s,fmt,args);
    // print out as usual
    while(*s) {
        /* convert newline to carrige return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}