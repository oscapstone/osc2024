#include "uart.h"
#include "shell.h"
#include "mailbox.h"

int get_hardware_info();

void main()
{
    // set up serial console
    uart_init();
    // echo hardware info
    get_hardware_info();
    // run shell
    while(1) {
        shell();
    }
}

int get_hardware_info() {
    // board revision
    mbox[0] = 7*4;
    mbox[1] = MBOX_REQUEST;
    // tags begin
    mbox[2] = MBOX_TAG_GETVERSION; // get board revision
    mbox[3] = 4; // Length of value in bytes
    mbox[4] = 0; // TAG_REQUEST_CODE level
    mbox[5] = 0;
    // tags end
    mbox[6] = MBOX_TAG_LAST;
    if (!mbox_call(MBOX_CH_PROP)) uart_puts("Unable to query serial!\n");
    uart_puts("board revision: ");
    uart_hex(mbox[5]);
    uart_puts("\r\n");
    // arm memory
    mbox[0] = 8*4;
    mbox[1] = MBOX_REQUEST;
    // tags begin
    mbox[2] = MBOX_TAG_GETARMMEM; // get ARM memory
    mbox[3] = 8; // Length of value in bytes
    mbox[4] = 0; // TAG_REQUEST_CODE level
    mbox[5] = 0;
    mbox[6] = 0;
    // tags end
    mbox[7] = MBOX_TAG_LAST;
    if (!mbox_call(MBOX_CH_PROP)) uart_puts("Unable to query serial!\n");
    uart_puts("ARM memory base address: ");
    uart_hex(mbox[5]);
    uart_puts("\r\n");
    uart_puts("ARM memory size: ");
    uart_hex(mbox[6]);
    uart_puts("\r\n");

    return 0;
}