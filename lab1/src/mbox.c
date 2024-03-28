#include "gpio.h"
#include "mbox.h"

/* Aligned to 16-byte boundary while we have 28-bits for VC */
volatile unsigned int __attribute__((aligned(16))) pt[64];

int mbox_call(mbox_channel_type channel, unsigned int value)
{
    // Add channel to lower 4 bit
    value &= ~(0xF);
    value |= channel;
    while ((*MBOX_STATUS & BCM_ARM_VC_MS_FULL) != 0)
        ;
    // Write to Register
    *MBOX_WRITE = value;
    while (1)
    {
        while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY)
            ;
        // Read from Register
        if (value == *MBOX_READ)
            return pt[1] == MBOX_REQUEST_SUCCEED;
    }
    return 0;
}

void get_mbox_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_CODE;
    pt[2] = MBOX_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("Hardware Revision\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_CODE;
    pt[2] = MBOX_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        uart_puts("ARM Memory Base Address\t: ");
        uart_2hex(pt[5]);
        uart_puts("\r\n");
        uart_puts("ARM Memory Size\t\t: ");
        uart_2hex(pt[6]);
        uart_puts("\r\n");
    }
}