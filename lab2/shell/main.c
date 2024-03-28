#include "header/uart.h"
#include "header/utils.h"
#include "header/mailbox.h"
#include "header/reboot.h"
#include "header/cpio.h"
#include "header/mem.h"
#include "header/devtree.h"

extern void *_dtb_ptr;

void shell() {
    uart_send_string("\nLab2Shell> ");
    char command[256];
    read_cmd(command);

    if (strcmp(command, "help")) {
        uart_send_string("\nhelp    : print this help menu ");
        uart_send_string("\nhello   : print Hello World! ");
        uart_send_string("\nmailbox : print mailbox info");
        uart_send_string("\nreboot  : reboot the device ");
        uart_send_string("\nls      : list files in cpio archive ");
        uart_send_string("\ncat     : print file content in cpio archive ");
        uart_send_string("\nmalloc  : test allocate memory ");
    } 
    else if (strcmp(command, "hello")) {
        uart_send_string("\nHello World! ");
    }
    else if (strcmp(command, "reboot")) {
        uart_send_string("\nRebooting... ");
        reset(200);
    }
    else if (strcmp(command, "mailbox"))
    {
        // get_board_revision();
        // get_memory_info();
        // get the board's unique serial number with a mailbox call
        mbox[0] = 8*4;                  // length of the message
        mbox[1] = MBOX_REQUEST;         // this is a request message
        
        mbox[2] = MBOX_TAG_GETMEMORY;   // get serial number command
        mbox[3] = 8;                    // buffer size
        mbox[4] = 0;
        mbox[5] = 0;                    // clear output buffer
        mbox[6] = 0;

        mbox[7] = MBOX_TAG_LAST;
        // send the message to the GPU and receive answer
        if (mbox_call(MBOX_CH_PROP)) {
            uart_send_string("\nARM memory base address : ");
            uart_hex(mbox[5]);
            uart_send_string("\r\n");

            uart_send_string("ARM memory size : ");
            uart_hex(mbox[6]);
            uart_send_string("\r\n");
        } else {
            uart_send_string("\nUnable to query serial!\n");
        }
        //
        mbox[0] = 7*4;                  // length of the message
        mbox[1] = MBOX_REQUEST;         // this is a request message
        
        mbox[2] = MBOX_TAG_GET_REVISION;   // get serial number command
        mbox[3] = 4;                    // buffer size
        mbox[4] = 0;
        mbox[5] = 0;                    // clear output buffer
        mbox[6] = MBOX_TAG_LAST;
        // send the message to the GPU and receive answer
        if (mbox_call(MBOX_CH_PROP)) {
            uart_send_string("board revision : ");
            uart_hex(mbox[5]);
            uart_send_string("\r\n");
        } else {
            uart_send_string("\nUnable to query serial!\n");
        }
    }
    else if (strcmp(command, "ls")) {
        cpio_ls();
    }
    else if (strcmp(command, "cat")) {
        cpio_cat();
    }
    else if (strcmp(command, "malloc")) {
        // test simple malloc (shorter string)
        char *str1 = (char *)simple_malloc(7);
        strncpy(str1, "Success", 7);
        str1[7] = '\0';
        uart_send_string("\r\n");
        uart_send_string("malloc test shorter string (7 bytes):");
        uart_send_string("\r\n");
        uart_send_string(str1);
        // test simple malloc (shorter string)
        char *str2 = simple_malloc(30);
        strncpy(str2, "Longer String Success Too", 30);
        str2[30] = '\0';
        uart_send_string("\r\n");
        uart_send_string("malloc test longer string (30 bytes):");
        uart_send_string("\r\n");
        uart_send_string(str2);
    }
    else {
        uart_send_string("\nCommand not found");
    }
}

void main() {
    // set up serial console
    uart_init();
    //
    uart_send_string("\nWelcome to Lab2 shell");
    //Use the API to get the address of initramfs instead of hardcoding it.
    fdt_traverse(initramfs_callback, _dtb_ptr);
    // set up simple allocator for dynamic memory
    mem_init();
    // say hello
    

    // shell
    while (1) {
        shell();
    }
}