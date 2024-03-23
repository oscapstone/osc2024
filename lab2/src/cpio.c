#include "../include/cpio.h"
#include "../include/mini_uart.h"
#include "../include/string_utils.h"
#include "../include/shell.h"

void cpio_ls()
{
    /* 1. Get the address of cpio_address. */
    char *addr = (char *)QEMU_CPIO_ADDR;

    /* 2. Use while loop to check the pathname of the file is "TRAILER!!!" or not. */
    while (my_strcmp((addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0) {

        /* 3. calculate the namesize. */
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned int namesize = hexstr2val((char *)header->c_namesize, 8);
        // uart_hex(namesize);

        /* 4. let pointer point to the start of filename. */
        addr += sizeof(struct cpio_header);
        uart_send_string(addr);
        uart_send_string("\r\n");

        /* 5. align the address to the start of the file. */
        addr += namesize;
        mem_align(&addr, 4);

        /* 6. calculate the filesize. */
        unsigned int filesize = hexstr2val((char *)header->c_filesize, 8);
        //uart_hex(filesize);
        
        // try to print out the file.
        // uart_send_string(addr);
        // break;

        /* 7. align the address to the start of another file header. */
        addr += filesize;
        mem_align(&addr, 4);
    }

}

void cpio_cat()
{
    /* 1. Get the address of cpio_address and set up the flag. */
    char *addr = (char *)QEMU_CPIO_ADDR;
    int flag = 0;

    /* 2. Wait user enter the file name. */
    char buffer[100];
    uart_send_string("Enter file name: ");
    read_command(buffer);
    // uart_send_string(buffer);

    /* 3. Use while loop to check the pathname of the file is "TRAILER!!!" or not. */
    while (my_strcmp((addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0) {

        /* 4. Calculate the namesize. */
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned int namesize = hexstr2val((char *)header->c_namesize, 8);
        // uart_hex(namesize);

        /* 5. Let pointer point to the start of filename. */
        addr += sizeof(struct cpio_header);
        // uart_send_string(addr);
        // uart_send_string("\r\n");

        /* 6. If the file name is the same as user input, flag -> 1. */
        if (my_strcmp(addr, buffer) == 0) 
            flag = 1;

        /* 7. Align the address to the start of the file. */
        addr += namesize;
        mem_align(&addr, 4);

        /* 8. Calculate the filesize. */
        unsigned int filesize = hexstr2val((char *)header->c_filesize, 8);
        //uart_hex(filesize);
        
        /* 9. Print out the file content. */
        if (flag) {
            uart_send_string(addr);
            uart_send_string("\r\n");
            break;
        }

        /* 10. align the address to the start of another file header. */
        addr += filesize;
        mem_align(&addr, 4);
    }

    /* If flag == 0, print out message. */
    if (!flag)
        uart_send_string("Please enter the correct filename. \r\n");

}