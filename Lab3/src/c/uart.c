/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *f
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
#include "uart.h"
#include "queue.h"
#include "math.h"
#include "tasklist.h"
#include "shell.h"
struct queue uart_write, uart_read;
/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void init_uart()
{
    register unsigned int reg;

    /* initialize UART */
    *AUX_ENABLE |= 1; /* enable mini UART */
    *AUX_MU_CNTL = 0; /* Disable transmitter and receiver during configuration. */

    *AUX_MU_IER = 0;    /* Disable interrupt */
    *AUX_MU_LCR = 3;    /* Set the data size to 8 bit. */
    *AUX_MU_MCR = 0;    /* Don’t need auto flow control. */
    *AUX_MU_BAUD = 270; /* 115200 baud */
    // *AUX_MU_IIR = 6;    /* No FIFO */
    *AUX_MU_IIR = 0xc6; /* No FIFO */

    /* map UART1 to GPIO pins */
    reg = *GPFSEL1;
    reg &= ~((7 << 12) | (7 << 15)); /* address of gpio 14, 15 */
    reg |= (2 << 12) | (2 << 15);    /* set to alt5 */

    *GPFSEL1 = reg;

    *GPPUD = 0; /* enable gpio 14 and 15 */
    reg = 150;
    while (reg--)
    {
        asm volatile("nop");
    }

    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    reg = 150;
    while (reg--)
    {
        asm volatile("nop");
    }

    *GPPUDCLK0 = 0; /* flush GPIO setup */

    *AUX_MU_CNTL = 3; // Enable the transmitter and receiver.
}

/**
 * Send a character
 */
void uart_send(unsigned int c)
{
    /* wait until we can send */
    // P.15 AUX_MU_LSR register shows the data(line) status
    // AUX_MU_LSR bit 5 => 0x20 = 00100000
    // bit 5 is set if the transmit FIFO can accept at least one byte.  
    // &0x20 can preserve 5th bit, if bit 5 set 1 can get !true = false leave loop
    // else FIFO can not accept at lease one byte then still wait 
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    //P.11 The AUX_MU_IO_REG register is primary used to write data to and read data from the
    //UART FIFOs.
    //communicate with(send to) the minicom and print to the screen 
    *AUX_MU_IO=c;
}

/**
 * Receive a character
 */
char uart_getc()
{
    char r;
    /* wait until something is in the buffer */
    //bit 0 is set if the receive FIFO holds at least 1 symbol.
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);
    /* convert carriage return to newline */
    return r;
}

/**
 * Read a whole line
 */
void uart_getline(char *buffer)
{
    char c;
    int counter = 0;

    while (1)
    {
        c = uart_getc();
        // delete
        if ((c == 127) && counter > 0)
        {
            counter--;
            uart_puts("\b \b");
        }
        // new line
        else if ((c == 10) || (c == 13))
        {
            buffer[counter] = '\0';
            uart_send(c);
            break;
        }
        // regular input
        else if (counter < 100)
        {
            buffer[counter] = c;
            counter++;
            uart_send(c);
        }
    }

    return;
}

/**
 * Display a string
 */
void uart_puts(char *s)
{
    while (*s)
    {
        /* convert newline to carrige return + newline */

        if (*s == '\n')
        {
            uart_send('\r');
            //uart_send('\n');
        }

        uart_send(*s++);
    }
}

char uart_async_read() {
    if(queue_empty(&uart_read))
        return;
    else{
        char c = queue_pop(&uart_read);
        return c;
    }
}

void uart_async_getline(char *file_name) {
    char c;
    int counter = 0;

    while (1)
    {
        c = uart_async_read();
        // delete
        if(c == 0)
            continue;
        if ((c == 127) && counter > 0)
        {
            counter--;
            uart_puts("\b \b");
        }
        // new line
        else if ((c == 10) || (c == 13))
        {
            file_name[counter] = '\0';
            uart_send(c);
            break;
        }
        // regular input
        else if (counter < 100)
        {
            file_name[counter] = c;
            counter++;
            uart_send(c);
        }
    }

    return;
}

void mini_uart_handler()
{
    //Mini Uart Interrupt Identify
    //AUX_MU_IIR[2:1]=01 =>Transmit holding register empty
    /*uart_puts("In mini_uart_handler\n");
    uart_hex(*AUX_MU_IIR);
    uart_send('\n');*/
    if (*AUX_MU_IIR & 2)
    { // can transmit
        char c = queue_pop(&uart_write);
        *AUX_MU_IO = c;
        if (queue_empty(&uart_write))
        {
            *AUX_MU_IER &= ~2; // disable transmit interrupt
        }
    }
    //AUX_MU_IIR[2:1]=10 =>Receiver holds valid byte
    else if (*AUX_MU_IIR & 4)
    { // can receive
        if (queue_full(&uart_read)){
            return;
        }
            
        char r = (char)(*AUX_MU_IO);
        queue_push(&uart_read, r);
        if(r == '\r'){
            uart_send('\n');
            create_task(shell_start, 2, 0, "uart");
        }
        else
            uart_send(r);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_puts("0x");
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x57:0x30;
        uart_send(n);
    }
}

/**
 * Display a binary value in decimal
 */
void uart_dec(unsigned int n) {
    int d = 0;
    int i = 1;
    while(n > 0){
        int r = n % 10;
        d += r * pow(10, i);
        n /= 10;
        i++;
    }
    while(i>1){
        d = d % pow(10, i);
        i--;
        uart_send((d/pow(10, i))+0x30);
    }
}
