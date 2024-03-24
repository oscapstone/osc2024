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

// Function to convert an integer to a character array
void int_to_char_array(int num, char* buffer) {
    int i = 0;
    int is_negative = 0;

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    // Convert each digit to character and store in reverse order
    do {
        buffer[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);

    // Add negative sign if necessary
    if (is_negative) {
        buffer[i++] = '-';
    }

    // Add null terminator
    buffer[i] = '\0';

    // Reverse the character array

}

void main(int argc, char* argv[]){
    int size=0;
    char *kernel=(char*)0x80000;
    char buffer[20]; // Buffer to store the character array
    
    // Convert integer to character array

    // set up serial console
    uart_init();
again:
    uart_send('A');
    size = uart_getc()-'0';
    size = size  + (uart_getc()-'0') * 10;
    size = size  + (uart_getc()-'0') * 100;
    size = size  + (uart_getc()-'0') * 1000;
    //size = size|(uart_getc()<<8); // getc loads 8 bits a time
    int_to_char_array(size, buffer);
    // while(size>0){
    //     uart_send((size%10)-'0');
    //     size/=10;
    // }
    uart_puts(buffer);
    goto again;
    
    // say hello. To reduce loader size I removed uart_puts()
// again:
//     uart_send('R');
//     uart_send('B');
//     uart_send('I');
//     uart_send('N');
//     uart_send('6');
//     uart_send('4');
//     uart_send('\r');
//     uart_send('\n');
//     // notify raspbootcom to send the kernel
//     uart_send(3);
//     uart_send(3);
//     uart_send(3);

//     // read the kernel's size
//     size=uart_getc();
//     size|=uart_getc()<<8;
//     size|=uart_getc()<<16;
//     size|=uart_getc()<<24;

//     // send negative or positive acknowledge
//     if(size<64 || size>1024*1024) {
//         // size error
//         uart_send('S');
//         uart_send('E');
//         goto again;
//     }
//     uart_send('O');
//     uart_send('K');

//     // read the kernel
//     while(size--) *kernel++ = uart_getc();

//     // restore arguments and jump to the new kernel.
//     asm volatile (
//         "mov x0, x10;"
//         "mov x1, x11;"
//         "mov x2, x12;"
//         "mov x3, x13;"
//         // we must force an absolute address to branch to
//         "mov x30, 0x80000; ret"
//     );
}
