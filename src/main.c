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
#include "sd.h"

#define CMD_LEN 128

// get the end of bss segment from linker
extern unsigned char _end;

void main()
{
    fdt_init();
    shell_init();

    // initialize EMMC and detect SD card type
    if(sd_init()==SD_OK) {
        // read the master boot record after our bss segment
        if(sd_readblock(0,&_end,1)) {
            // dump it to serial console
            uart_dump(&_end);
        }
    }

    // get_board_revision();
    // get_memory_info();

    fdt_traverse(initramfs_callback);

    // uart_hex(return_available());
    // uart_send('\n');
    char *string1 = simple_malloc(1);
    uart_hex((unsigned int) string1);
    uart_send('\n');

    char *string2 = simple_malloc(sizeof(char) * 20);
    uart_hex((unsigned int) string2);
    uart_send('\n');

    char *string3 = (char *) simple_malloc(sizeof(char) * 20);
    uart_hex((unsigned int) string3);
    uart_send('\n');

    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}
