#include "shell.h"
#include "uart.h"
#include "string.h"
#include "cpio.h"
#include "mbox.h"
#include "utils.h"
void HardwareInformation()
{
    // get the board's unique serial number with a mailbox call
    mbox[0] = 8 * 4;        // length of the message
    mbox[1] = MBOX_REQUEST; // this is a request message

    mbox[2] = MBOX_TAG_GETSERIAL; // get serial number command
    mbox[3] = 8;                  // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
    {
        uart_puts("My serial number is: ");
        uart_hex(mbox[6]);
        uart_hex(mbox[5]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Unable to query serial!\n");
    }

    mbox[0] = 8 * 4;        // length of the message
    mbox[1] = MBOX_REQUEST; // this is a request message

    mbox[2] = MBOX_TAG_GETBOARDREVISION; // get board revision command
    mbox[3] = 4;                         // buffer size
    mbox[4] = 4;
    mbox[5] = 0; // clear output buffer
    mbox[6] = MBOX_TAG_LAST;

    // mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
    {
        uart_puts("My board revision is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Unable to board revision!\n");
    }

    mbox[0] = 8 * 4;        // length of the message
    mbox[1] = MBOX_REQUEST; // this is a request message

    mbox[2] = MBOX_TAG_ARMMEMORY; // get bGet ARM memory command
    mbox[3] = 8;                  // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;

    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
    {
        uart_puts("base address is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("My size is: ");
        uart_hex(mbox[6]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Unable to base address!\n");
    }
}
void shellStart()
{
    while (1)
    {
        uart_puts("# ");
        char *command;
        uart_getcommand(command);
        if (!strcmp(command, "help"))
        {
            uart_puts("help\t: print this help menu\nhello\t: print Hello World!\nreboot\t: reboot the device\n");
        }
        if (!strcmp(command, "hello"))
        {
            uart_puts("Hello World!\n");
        }
        if (!strcmp(command, "reboot"))
        {
            uart_puts("Start reboot...\n");
            reset(100);
            while (1)
                ;
        }
        if (!strcmp(command, "mailbox"))
        {
            HardwareInformation();
        }
        if (!strcmp(command, "ls"))
        {
            cpioLs();
        }
        if (!strcmp(command, "cat"))
        {
            uart_puts("File name: ");
            char *fileName;
            uart_getcommand(fileName);
            cpioCat(fileName);
        }
        if (!strcmp(command, "malloc"))
        {
            void *s = simpleMalloc(8);
            uart_puts("Allocated 8 bytes of memory\n");
            uart_hex((unsigned int)s);
            uart_puts("\n");
            s = simpleMalloc(16);
            uart_puts("Allocated 16 bytes of memory\n");
            uart_hex((unsigned int)s);
            uart_puts("\n");
            s = simpleMalloc(32);
            uart_puts("Allocated 32 bytes of memory\n");
            uart_hex((unsigned int)s);
            uart_puts("\n");
        }
    }
}