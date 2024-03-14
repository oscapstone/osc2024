#include "uart.h"
#include "mbox.h"
#include "lib_func.h"

void uart_puts_boardrev()
{
    // get the board's unique serial number with a mailbox call
    mbox[0] = 8 * 4;              // length of the message
    mbox[1] = MBOX_REQUEST;       // this is a request message
    mbox[2] = MBOX_TAG_GET_BOARD_REVISION; // get board revision
    mbox[3] = 8;                  // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
    {
        uart_puts("\rThe board revision is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("\rUnable to query!\n");
    }
}

void uart_puts_armmemory()
{
    // get the board's unique serial number with a mailbox call
    mbox[0] = 8 * 4;              // length of the message
    mbox[1] = MBOX_REQUEST;       // this is a request message
    mbox[2] = MBOX_TAG_ARM_MEMORY; // get arm memory command
    mbox[3] = 8;                  // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP))
    {
        uart_puts("\rARM memory base address: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("\rARM memory size: ");
        uart_hex(mbox[6]);
        uart_puts("\n");
    }
    else
    {
        uart_puts("\rUnable to query!\n");
    }
}

//REBOOT
#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}

void main()
{
    // set up serial console
    uart_init();

    uart_puts("\rSimple Shell 1.0\n");
    char inputBuffer[64];
    int i = 0;
    //clear the line because of the random character arriving
    // uart_getc();

    while (1)
    {
        // Read input until newline of buffer is full
        for (i = 0; i < sizeof(inputBuffer) - 1; i++)
        {
            char c = uart_getc();
            if ((c >= 32 && c <= 126) || c == '\n') // Check for valid ASCII characters or newline
            {
                inputBuffer[i] = c;
                uart_send(inputBuffer[i]); // Echo the valid character
                if (inputBuffer[i] == '\n')
                {
                    break; // Exit the loop if newline is encountered
                }
            }
            else
            {
                i--; // Decrement the index to overwrite any invalid characters
            }
        }
        inputBuffer[i] = '\0';

        // Check commands
        if (strcmp(inputBuffer, "help") == 0)
        {
            uart_puts("\r# help\nhelp\t: print this help menu\nhello\t: print Hello World!\nreboot\t: reboot the device\ninfo\t: get basic hardware info\n");
        }
        else if (strcmp(inputBuffer, "hello") == 0)
        {
            uart_puts("\rHello World!\n");
        }
        else if (strcmp(inputBuffer, "info") == 0)
        {
            uart_puts_boardrev();
            uart_puts_armmemory();
        }
        else if (strcmp(inputBuffer, "reboot") == 0){
            reset(5);
        } 
        else {
            uart_puts("\runknown command\n");
        }

        // reset buffer
        memset(inputBuffer, 0, sizeof(inputBuffer));
    }
    // echo everything back
    // while(1) {
    //     uart_send(uart_getc());
    // }
}