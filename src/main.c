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

#include "uart.h"
#include "power.h"
#include "shell.h"
#include "mbox.h"
#include "stdlib.h"
#include "dtb.h"
#include "initrd.h"

#define CMD_LEN 128

void main()
{
    shell_init();
    // uart_async_init();
    fdt_init();
    fdt_traverse(initramfs_callback);

    // read the current level from system register. If we were el0 with spsr_el1 == 0, we can't access CurrentEL.
    unsigned long el;
    asm volatile ("mrs %0, CurrentEL\n\t" : "=r" (el));
    uart_puts("Current EL is: ");
    uart_hex((el >> 2) & 3);
    uart_puts("\n");

    /* Switch to el0 before running shell */
    asm volatile("bl from_el1_to_el0");
    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}
