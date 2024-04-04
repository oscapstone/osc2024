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
#include "sprintf.h"
#include "queue.h" // for async uart

extern unsigned char _end;

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    AUX->AUX_ENABLES = 1;           // enable mini uart (this also enables access to it registers)
    AUX->AUX_MU_CNTL_REG = 0;       // Disable transmitter and receiver during configuration.
    AUX->AUX_MU_LCR_REG = 3;        // 8 bits
    AUX->AUX_MU_MCR_REG = 0;        // RTS line high
    AUX->AUX_MU_IER_REG = 0;        // Disable transmit/receive interrupts.
    AUX->AUX_MU_IIR_REG = 0xc6;     // Clear the transmit/receive FIFO.
    AUX->AUX_MU_BAUD_REG = 270;     // 115200 baud
    /* map UART1 to GPIO pins */
    r = GPIO->GPFSEL[1];
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    GPIO->GPFSEL[1] = r;
    GPIO->GPPUD = 0;      // enable pins 14 and 15
    r = 150;
    while (r--)
        asm volatile("nop");
    GPIO->GPPUDCLK[0] = (1 << 14) | (1 << 15);
    r = 150;
    while (r--)
        asm volatile("nop");

    GPIO->GPPUDCLK[0] = 0;    // flush GPIO setup
    AUX->AUX_MU_CNTL_REG = 3; // enable Tx, Rx
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    do {
        asm volatile("nop");
    } while (!(AUX->AUX_MU_LSR_REG & 0x20));
    /* write the character to the buffer */
    AUX->AUX_MU_IO_REG = c;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (!(AUX->AUX_MU_LSR_REG & 0x01));
    /* read it and return */
    r = (char) (AUX->AUX_MU_IO_REG);
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

/**
 * Display a string
 */
void printf(char *fmt, ...)
{
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
int check_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

/**
 * Dump memory
 */
void uart_dump(void *ptr)
{
    unsigned long a,b,d;
    unsigned char c;
    for(a=(unsigned long)ptr;a<(unsigned long)ptr+512;a+=16) {
        uart_hex(a); uart_puts(": ");
        for(b=0;b<16;b++) {
            c=*((unsigned char*)(a+b));
            d=(unsigned int)c;d>>=4;d&=0xF;d+=d>9?0x37:0x30;uart_send(d);
            d=(unsigned int)c;d&=0xF;d+=d>9?0x37:0x30;uart_send(d);
            uart_send(' ');
            if(b%4==3)
                uart_send(' ');
        }
        for(b=0;b<16;b++) {
            c=*((unsigned char*)(a+b));
            uart_send(c<32||c>=127?'.':c);
        }
        uart_send('\r');
        uart_send('\n');
    }
}

/**
 * Enable both receive and transmit interrupt
 * Enable mini UART receive interrupt (at pending bit 29)
 * AUX->AUX_MU_IER_REG: Set bit 0 to enable receive interrupt. Set bit 1 to enable transmit interrupt.
 * receive interrupt occurs when the receive FIFO has valid data.
 * transmit interrupt occurs when the transmit FIFO is empty.
*/
void uart_async_init()
{
    AUX->AUX_MU_IER_REG |= (1 << 0); // enable receive interrupt. transmit interrupt will be enabled when we write data to write_buffer.
    IRQ->ENABLE_IRQS1 |= (1 << 29); // set IRQ_ENABLE to enable mini UART interrupt (AUX interrupt) which is at pending bit 29.
}

char uart_async_getc(void)
{
    if (is_empty(&read_buffer))
        return 0;
    return dequeue_char(&read_buffer);
}

void uart_async_send(unsigned int c)
{
    enqueue_char(&write_buffer, c);
    AUX->AUX_MU_IER_REG |= (1 << 1); // enable transmit interrupt
}

void uart_async_puts(char *s)
{
    while(*s) {
        enqueue_char(&write_buffer, *s++);
    }
    AUX->AUX_MU_IER_REG |= (1 << 1); // enable transmit interrupt
}

/**
 * Get all characters in read_buffer.
*/
int uart_async_gets(char *buf)
{
    int i = 0;
    while (!is_empty(&read_buffer))
        buf[i++] = dequeue_char(&read_buffer);
    buf[i] = '\0';
    return i;
}

/* uart_tasklet: do enqueue read buffer */
void uart_tasklet(unsigned long data)
{
    char c = (char)data;
    enqueue_char(&read_buffer, c);
}