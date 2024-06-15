#include "peripheral/mailbox.h"
#include "mini_uart.h"
#include "utils.h"

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mbox_buf[36];

/**
 * Make a mailbox call. Returns 1 on success, 0 on failure.
 */
int mailbox_call(unsigned char ch, unsigned int* mbox)
{
    unsigned int r =
        (((unsigned int)((unsigned long)mbox) & ~0xF) | (ch & 0xF));

    // wait until we can write to the mailbox
    while (get32(MBOX_STATUS) & MBOX_FULL)
        ;
    // write the address of our message to the mailbox with channel identifier
    put32(MBOX_WRITE, r);

    // now wait for the response
    while (1) {
        // is there a response?
        while (get32(MBOX_STATUS) & MBOX_EMPTY)
            ;
        // is it a response to our message?
        if (r == get32(MBOX_READ))
            // is it a valid successful response?
            return *(volatile unsigned int*)(mbox + 1) == MBOX_RESPONSE;
    }
    return 0;
}

int mbox_get_board_revision(void)
{
    mbox_buf[0] = 7 * 4;  // buffer size in bytes
    mbox_buf[1] = MBOX_REQUEST;
    // tags begin
    mbox_buf[2] = MBOX_TAG_GET_REVISION;  // tag identifier
    mbox_buf[3] = 4;  // maximum of request and response value buffer's length.
    mbox_buf[4] = MBOX_TAG_REQUEST_CODE;
    mbox_buf[5] = 0;  // value buffer
    // tags end
    mbox_buf[6] = MBOX_TAG_LAST;

    return mailbox_call(MBOX_CH_PROP, (unsigned int*)mbox_buf);
}

int mbox_get_arm_memory(void)
{
    mbox_buf[0] = 8 * 4;  // buffer size in bytes
    mbox_buf[1] = MBOX_REQUEST;
    // tags begin
    mbox_buf[2] = MBOX_TAG_GET_ARM_MEMORY;  // tag identifier
    mbox_buf[3] = 8;  // maximum of request and response value buffer's length.
    mbox_buf[4] = MBOX_TAG_REQUEST_CODE;
    mbox_buf[5] = 0;  // value buffer
    mbox_buf[6] = 0;  // value buffer
    // tags end
    mbox_buf[7] = MBOX_TAG_LAST;

    return mailbox_call(MBOX_CH_PROP, (unsigned int*)mbox_buf);
}


void print_board_revision(void)
{
    if (!mbox_get_board_revision()) {
        uart_send_string("Unable to get board revision\n");
        return;
    }

    uart_send_string("Board revision: ");
    uart_send_string("0x");
    uart_send_hex(mbox_buf[5]);
    uart_send_string("\n");
}

void print_arm_memory(void)
{
    if (!mbox_get_arm_memory()) {
        uart_send_string("Unable to get arm memory\n");
        return;
    }

    uart_send_string("ARM base address: ");
    uart_send_string("0x");
    uart_send_hex(mbox_buf[5]);
    uart_send_string("\n");

    uart_send_string("ARM memory size: ");
    uart_send_string("0x");
    uart_send_hex(mbox_buf[6]);
    uart_send_string(" bytes\n");
}
