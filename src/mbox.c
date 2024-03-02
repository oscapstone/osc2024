#include "mbox.h"
#include "uart.h"
#include "utli.h"

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mbox[36];

#define MBOX_BASE (MMIO_BASE + 0xb880)
/* Regs */
#define MBOX_READ ((volatile unsigned int *)(MBOX_BASE + 0x00))
#define MBOX_STATUS ((volatile unsigned int *)(MBOX_BASE + 0x18))
#define MBOX_WRITE ((volatile unsigned int *)(MBOX_BASE + 0x20))

/* tags */
#define MBOX_TAG_BOARD_REVISION 0x00010002
#define MBOX_TAG_GETSERIAL 0x10004
#define MBOX_TAG_ARM_MEM 0x10005
#define MBOX_TAG_LAST 0x00000000

/* CODE */
#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

void get_arm_base_memory_sz() {
    mbox[0] = 8 * 4;            // length of the message
    mbox[1] = REQUEST_CODE;     // this is a request message
    mbox[2] = MBOX_TAG_ARM_MEM; // get serial number command
    mbox[3] = 8;                // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        uart_printf("memory base address: %08x\n", mbox[6]);
        uart_printf("memory size: %d bytes\n", mbox[5]);
    } else {
        uart_printf("Unable to query arm memory and size..\n");
    }
}

void get_board_serial() {
    // get the board's unique serial number with a mailbox call
    mbox[0] = 8 * 4;              // length of the message
    mbox[1] = REQUEST_CODE;       // this is a request message
    mbox[2] = MBOX_TAG_GETSERIAL; // get serial number command
    mbox[3] = 8;                  // buffer size
    mbox[4] = 8;
    mbox[5] = 0; // clear output buffer
    mbox[6] = 0;
    mbox[7] = MBOX_TAG_LAST;

    if (mbox_call(MBOX_CH_PROP)) {
        uart_printf("My serial number is: 0x%08x%08x\n", mbox[6], mbox[5]);
    } else {
        uart_printf("Unable to query serial number..\n");
    }
}

void get_board_revision() {
    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = MBOX_TAG_BOARD_REVISION; // tag identifier
    mbox[3] = 4;                       // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;             // value buffer
    mbox[6] = MBOX_TAG_LAST; // tags end

    if (mbox_call(MBOX_CH_PROP)) {
        uart_printf("Board revision number: 0x%x\n", mbox[5]); // it should be 0xa020d3 for rpi3 b+
    } else {
        uart_printf("Unable to query board revision..\n");
    }
}

int mbox_call(unsigned char ch) {
    unsigned int r = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
    /* wait until we can write to the mailbox */
    do {
        asm volatile("nop");
    } while (get(MBOX_STATUS) & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */

    set(MBOX_WRITE, r);
    /* now wait for the response */

    do {
        asm volatile("nop");
    } while (get(MBOX_STATUS) & MBOX_EMPTY);
    /* is it a response to our message? */
    if (r == get(MBOX_READ))
        /* is it a valid successful response? */
        return (mbox[1] == REQUEST_SUCCEED);

    return 0;
}
