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
#include "include/cpio.h"


void main(int argc, char* argv[]){
   asm volatile(
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
      "mov x0, x10;"
   );
   // uart_init();
   // uart_puts("uart_not_bugged\n");
   // cpio_ls();
   // cpio_cat("hello2.txt");
   // cpio_ls();
   // cpio_cat("hello.txt");
   int* data = (int *)simple_malloc((unsigned long) sizeof(int)*8);
   int* b = (int *)simple_malloc((unsigned long) sizeof(int)*8);
   uart_puts("exit normally\n");
   // asm volatile(
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "mov x0, x10;"
   //    "ret;"
   // );
   return;
}
