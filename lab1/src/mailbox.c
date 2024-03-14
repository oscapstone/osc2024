#include "mailbox.h"

#include "mini_uart.h"
#include "peripherals/base.h"
#include "peripherals/gpio.h"


#define VIDEOCORE_MBOX (PBASE + 0x0000B880)
#define MBOX_READ      ((volatile unsigned int*) (VIDEOCORE_MBOX + 0x0))  //Mailbox 0 read register
#define MBOX_STATUS    ((volatile unsigned int*) (VIDEOCORE_MBOX + 0x18)) //Mailbox 0 status register
#define MBOX_WRITE     ((volatile unsigned int*) (VIDEOCORE_MBOX + 0x20)) //Mailbox 1 write register
#define MBOX_RESPONSE  0x80000000   // response code: request successful
#define MBOX_FULL      0x80000000
#define MBOX_EMPTY     0x40000000

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mbox[36];

void prepare_mbox_message(unsigned int tag, unsigned int buffer_size) {
    // Common header
    mbox[0] = buffer_size;    // Total message size in bytes
    mbox[1] = MBOX_REQUEST;   // This is a request message
    // tags begin
    mbox[2] = tag;   // tag identifier (get_board_revision or get_ARM_memory)
    mbox[3] = 8;     // maximum of request and response value buffer's length.
    mbox[4] = 8;   // tag request code 
    mbox[5] = 0;   // value buffer
    mbox[6] = 0;   // value buffer
    // tags end
    mbox[7] = MBOX_TAG_LAST;
}

/**
 * Make a mailbox call to specified channel. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch) {
    /* 1. clear the lowest four bits, ensure the address is 16 bits aligned, Combine the message address (upper 28 bits) with channel number (lower 4 bits)*/
    unsigned int r = (((unsigned int) ((unsigned long) &mbox) & ~0xF) | (ch & 0xF));
    
    /* 2. Check if Mailbox 0 status register’s full flag is set ,write to the mailbox */
    while (1) {
        if (!(*MBOX_STATUS & MBOX_FULL)) //If not, then you can write to Mailbox 1 Read/Write register.
            break;
    }
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r; // 3. Write to Mailbox 1 Write register.
    /* now wait for the response */
    while (1) {
        /* is there a response? */
        while (1) {
            if (!(*MBOX_STATUS & MBOX_EMPTY)) // 4. Check if Mailbox 0 status register’s empty flag is set.
                break;
        }
        //5. read from Mailbox 0 Read register.
        if (r == *MBOX_READ)
            /* use MBOX_READ register to check if it is a response to our message, check if it is a valid successful response */
            return mbox[1] == MBOX_RESPONSE;
            // 6. Check if the value is the same as you wrote in step 1,
    }
    return 0;
}

void get_board_revision() {
    prepare_mbox_message(GET_BOARD_REVISION, 8 * 4);   // Prepare message for board revision

    if (mbox_call(MBOX_CH_PROP)) { //ch8
        uart_puts("My revision is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
}
void get_ARM_memory() {
    prepare_mbox_message(GET_ARM_MEMORY, 8 * 4);   // Prepare message for ARM memory

    if (mbox_call(MBOX_CH_PROP)) {
        uart_puts("My ARM Memory's base address is: ");
        uart_hex(mbox[5]);
        uart_puts("\n");
        uart_puts("My ARM Memory's size in bytes is: ");
        uart_hex(mbox[6]);
        uart_puts("\n\n");
    } else {
        uart_puts("Unable to query serial!\n");
    }
}