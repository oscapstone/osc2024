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
#include "memblock.h"
#include "exec.h"
#include "mm.h"
#include "vfs.h"
#include "initramfs.h"

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
        int fs = ALIGN(hex2bin(header->filesize, 8), 4);
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
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
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
        int fs = ALIGN(hex2bin(header->filesize, 8), 4);
        // print out filename
        uart_puts(buf + sizeof(cpio_f));      // filename
        uart_puts("\n");
        // jump to the next file
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
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
        int fs = ALIGN(hex2bin(header->filesize, 8), 4);

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
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
    }
    uart_puts("File not found\n");
}

/* Return the user program execution address */
void *_initrd_usr_prog(char *cmd, unsigned int *size)
{
    char *buf = cpio_base, *prog_addr, *prog_page;
    int ns, fs;
    cpio_f *header;

    if (memcmp(buf, "070701", 6)) // check if it's a cpio newc archive
        return NULL;
    
    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(cpio_f), "TRAILER!!", 9)) { // check if it's a cpio newc archive and not the last file
        header = (cpio_f*) buf;
        ns = hex2bin(header->namesize, 8);
        fs = ALIGN(hex2bin(header->filesize, 8), 4);

        // check filename with buffer
        if (!strcmp(cmd, buf + sizeof(cpio_f))) {
            if (fs == 0) {
                printf("\nUser program is empty\n");
                return NULL;
            } else {
                printf("\nFound user_program: %s\n", buf + sizeof(cpio_f));
                *size = fs;
                /* get program start address */
                prog_addr = buf + ALIGN(sizeof(cpio_f) + ns, 4);

                /* Allocate a page and copy the user program to the page. */
                prog_page = (char *) kmalloc(fs);
                memmove(prog_page, prog_addr, fs);

                return prog_page;
            }
        }
        // jump to the next file
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
    }
    return NULL;
}

/* Execute the user program with program name as input parameter. */
void initrd_usr_prog(char *cmd)
{
    char *prog;
    unsigned int size;

    prog = (char *) _initrd_usr_prog(cmd, &size);
    if (prog != NULL)
        do_exec((void (*)(void)) prog, size);
    else
        uart_puts("File not found\n");
    return;
}

void initramfs_callback(fdt_prop *prop, char *node_name, char *property_name)
{
    if (!strcmp(node_name, "chosen") && !strcmp(property_name, "linux,initrd-start")) {
        uint32_t load_addr = *((uint32_t *)(prop + 1));
        cpio_base = (char *)(phys_to_virt((unsigned long)bswap_32(load_addr)));
        printf("-- cpio_base: %x\n", cpio_base);
    }
}

void initrd_reserve_memory(void)
{
    cpio_f *header;
    char *buf = cpio_base;
    int ns, fs;

    /* check if it's a cpio newc archive. */
    if (memcmp(buf, "070701", 6))
        return;

    /* if it's a cpio newc archive. Cpio also has a trailer entry */
    while(memcmp(buf + sizeof(cpio_f), "TRAILER!!", 9)) {
        header = (cpio_f*) buf;
        ns = hex2bin(header->namesize, 8);
        fs = ALIGN(hex2bin(header->filesize, 8), 4);

        /* jump to the next file */
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
    }
    /* reserve the memory from cpio_base to buf + sizeof(cpio_f) + ns */
    buf += ALIGN(sizeof(cpio_f) + 10, 4);
    memblock_reserve((unsigned long) virt_to_phys((unsigned long)cpio_base), ALIGN(buf - cpio_base, 8));
}

/* Iterate all the files in .cpio and put it under initramfs_inode */
void initramfs_init(struct initramfs_inode *dir)
{
    cpio_f *header;
    char *buf = cpio_base, *file_name, *data;
    struct vnode *file_vnode;
    struct initramfs_inode *file_inode;
    int idx = 0, ns, fs;

    if (memcmp(buf, "070701", 6))
        return;

    // if it's a cpio newc archive. Cpio also has a trailer entry
    while(!memcmp(buf, "070701", 6) && memcmp(buf + sizeof(cpio_f), "TRAILER!!", 9)) {
        header = (cpio_f*) buf;
        ns = hex2bin(header->namesize, 8);
        fs = ALIGN(hex2bin(header->filesize, 8), 4);
        file_name = buf + sizeof(cpio_f);
        data = buf + ALIGN(sizeof(cpio_f) + ns, 4);

        /* Ignore directory */
        if (fs == 0)
            goto next_file;
        
        /* Create file vnode */
        file_vnode = initramfs_create_vnode(0, FSNODE_TYPE_FILE);
        file_inode = file_vnode->internal;
        /* Update initramfs inode information */
        strcpy(file_inode->name, file_name);
        file_inode->data = data;
        file_inode->data_size = fs;
        dir->childs[idx++] = file_vnode;

next_file: // jump to the next file
        buf += (ALIGN(sizeof(cpio_f) + ns, 4) + fs);
    }
}