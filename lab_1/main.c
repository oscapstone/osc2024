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
#define MAX_BUFFER 10
#include "uart.h"
#include "utils.h"
#include "mbox.h"
void main()
{
    // set up serial console
    uart_init();

    // get the board's unique serial number with a mailbox call
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_REVISION;   // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("Revision number is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }

    // get the board's unique serial number with a mailbox call
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_MEMORY;   // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("Memory base address is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("Memory size is: ");
        uart_hex(mbox[6]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }

    while(1){
        char command[MAX_BUFFER];
        char c = '\0';
        int length=0;
        for(int i=0; i<MAX_BUFFER; i++){
            command[i] = '\0';
        }
        for(length=0; c != '\n' && length<MAX_BUFFER; length++){
            c = uart_getc();
            command[length] = c;
            uart_send(c);
        }
        command[length==MAX_BUFFER?length-2:length-1] = '\0';
        uart_puts("\r");
        if(strcmp(command, "help") == 0){
            uart_puts("help\t: print this help menu\n");
            uart_puts("hello\t: print hello world!\n");
            uart_puts("reboot\t: reboot the device\n");
        }
        else if(strcmp(command, "hello") == 0){
            uart_puts("Hello World!\n");
        }
        else if(strcmp(command, "reboot") == 0){
            reset();
        }
    }
}
