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
#include "shell.h"
#include "string.h"
#include "file.h"
#include "initrd.h"
#include "stdint.h"

#define buf_size 128

char *cpio_base;

/**
 * List the contents of an archive
 */
void initrd_list()
{
    char *buf = cpio_base;
    if (memcmp(buf, "070701", 6))
        return;
    uart_puts("Type     Offset   Size     Access rights\tFilename\n");

    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf, "070701",6) && memcmp(buf + sizeof(cpio_f),"TRAILER!!",9)) {
        cpio_f *header = (cpio_f*) buf;
        int ns = hex2bin(header->namesize, 8);
        int fs = hex2bin(header->filesize, 8);
        // print out meta information
        uart_hex(hex2bin(header->mode, 8));  // mode (access rights + type)
        uart_send(' ');
        uart_hex((unsigned int)((unsigned long) buf) + sizeof(cpio_f) + ns);
        uart_send(' ');
        uart_hex(fs);                       // file size in hex
        uart_send(' ');
        uart_hex(hex2bin(header->uid, 8));   // user id in hex
        uart_send('.');
        uart_hex(hex2bin(header->gid, 8));   // group id in hex
        uart_send('\t');
        uart_puts(buf + sizeof(cpio_f));      // filename
        uart_puts("\n");
        // jump to the next file
        buf += (sizeof(cpio_f) + ns + fs);
    }
}

/**
 * List the filenames of an archive
 */
void initrd_ls()
{
    char *buf = cpio_base;
    if (memcmp(buf, "070701", 6))
        return;
    uart_puts(".\n");

    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(cpio_f), "TRAILER!!", 9)) {
        cpio_f *header = (cpio_f*) buf;
        int ns = hex2bin(header->namesize, 8);
        int fs = hex2bin(header->filesize, 8);
        // print out filename
        uart_puts(buf + sizeof(cpio_f));      // filename
        uart_puts("\n");
        // jump to the next file
        buf += (sizeof(cpio_f) + ns + fs);
    }
}

void initrd_cat()
{
    char *buf = cpio_base;
    if (memcmp(buf, "070701", 6)) // check if it's a cpio newc archive
        return;
    char buffer[buf_size];

    // get file name
    uart_puts("filename: ");
    shell_input(buffer);

    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf,"070701",6) && memcmp(buf+sizeof(cpio_f),"TRAILER!!",9)) {
        cpio_f *header = (cpio_f*) buf;
        int ns = hex2bin(header->namesize, 8);
        int fs = hex2bin(header->filesize, 8);

        // check filename with buffer
        if (!strcmp(buffer, buf + sizeof(cpio_f))) {
            if (fs == 0) {
                uart_send('\n');
                uart_puts("This is a directory\n");
                return;
            } else {
                uart_send('\n');
                readfile(buf + sizeof(cpio_f) + ns, fs);
                return;
            }
        }

        // jump to the next file
        buf += (sizeof(cpio_f) + ns + fs);
    }
    uart_puts("File not found\n");
}

void initrd_usr_prog(char *cmd)
{
    char *buf = cpio_base;
    if (memcmp(buf, "070701", 6)) // check if it's a cpio newc archive
        return;
    
    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(cpio_f), "TRAILER!!", 9)) { // check if it's a cpio newc archive and not the last file
        cpio_f *header = (cpio_f*) buf;
        int ns = hex2bin(header->namesize, 8);
        int fs = hex2bin(header->filesize, 8);

        // check filename with buffer
        if (!strcmp(cmd, buf + sizeof(cpio_f))) {
            if (fs == 0) {
                uart_send('\n');
                uart_puts("user_program is empty.\n");
                return;
            } else {
                uart_puts("\nInto user_program: ");
                uart_puts(buf + sizeof(cpio_f));
                uart_puts("\nAddress: ");
                uart_hex((int) buf + sizeof(cpio_f));
                uart_send('\n');
                // get program start address
                char *prog_addr = buf + sizeof(cpio_f) + ns;
                
                char *program_position = (char *)0x10A0000;

                while (fs--)
                {
                    *program_position = *prog_addr;
                    program_position++;
                    prog_addr++;
                }
                asm volatile(
                    "mov x0, 0          \n\t"
                    "msr spsr_el1, x0       \n\t"
                    "mov x0, 0x10A0000      \n\t"
                    "msr elr_el1, x0        \n\t"
                    "mov x0, 0x60000        \n\t"
                    "msr sp_el0, x0         \n\t"
                    "eret                   \n\t");
            }
        }
        // jump to the next file
        buf += (sizeof(cpio_f) + ns + fs);
    }
    uart_puts("File not found\n");
}

void initramfs_callback(fdt_prop *prop, char *node_name, char *property_name)
{
    if (!strcmp(node_name, "chosen") && !strcmp(property_name, "linux,initrd-start")) {
        uint32_t load_addr = *((uint32_t *)(prop + 1));
        cpio_base = (char *) bswap_32(load_addr);
        uart_puts("cpio_base: ");
        uart_hex((int) cpio_base);
        uart_send('\n');
    }
}