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
#include "include/uart.h"
#include "include/utils.h"
#include "include/mbox.h"

void load_img(){
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    uart_puts("enter kernel size now:");
    for(int i=0; i<4; i++) 
	    size_buffer[i] = uart_getc();
    uart_puts("size-check correct\n");

    uart_puts("Kernel size received: ");
    uart_hex(size);
    uart_puts("\n");

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_getc();

    uart_puts("kernel-loaded\n");
    return;

}

void relocator(int argc, char* argv[]){
    asm volatile(
       "mov x10, x0"
    );
    uart_init();
    load_img();
    asm volatile(
       "mov x0, x10;"
       "mov x30, 0x80000;"
       "ret;"
    );
}
