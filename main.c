#include "uart.h"
#include "shell.h"
#include "mbox.h"

int main()
{
    // set up serial console
    uart_init();
    
    // get board revision    
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_GETREVISION; // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("Board Revision : ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
    
    // get board revision    
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // this is a request message
    
    mbox[2] = MBOX_TAG_GETARMMEM; // get serial number command
    mbox[3] = 8;                    // buffer size
    mbox[4] = 8;
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;
    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("ARM memory base address in bytes : ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("ARM memory size in bytes : ");
        uart_hex(mbox[6]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }

    // start shell
    shell_start();

    return 0;
}
